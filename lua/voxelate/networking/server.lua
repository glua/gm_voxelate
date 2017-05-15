local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

local SharedRouter = runtime.require("./shared").Router
runtime.oop.extend(Router,SharedRouter)

local bitbuf = runtime.require("../bitbuffer")

function Router:__ctor(voxelate)
	self.super:__ctor(voxelate)

	self.PUIDs = setmetatable({},{__mode = "v"}) -- {[PUID] = Player}
	self.PUIDsEx = setmetatable({},{__mode = "k"}) -- {[Player] = PUID}
	self.PeerIDs = setmetatable({},{__mode = "v"}) -- {[PeerID] = Player}
	self.PeerIDsEx = setmetatable({},{__mode = "k"}) -- {[Player] = PUID}

	self.PeerAddress = {} -- {[PeerID] = IPAddress}

	-- timeout helpers
	local function addPlayerTimeout(ply,timeoutID,time,callback)
		timer.Create(string.format("timeout_%s_%s",ply:SteamID(),timeoutID),time,1,callback)
	end

	local function removePlayerTimeout(ply,timeoutID)
		timer.Remove(string.format("timeout_%s_%s",ply:SteamID(),timeoutID))
	end

	hook.Add("VoxNetworkConnect","Voxelate.Networking",function(peerID,address)

		self.voxelate.io:PrintDebug("A new ENet Peer [%d] has connected from %s",peerID,address)
		self.PeerAddress[peerID] = address
	end)

	hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function(peerID)
		self.PeerAddress[peerID] = nil

		local PUID = self.PUIDsEx[ply]

		if PUID then
			self.PUIDs[PUID] = nil
		end

		local ply = self.PeerIDs[peerID]
		self.PeerIDs[peerID] = nil

		if ply then
			self.PUIDsEx[ply] = nil
			self.PeerIDsEx[ply] = nil

			if IsValid(ply) then
				self.voxelate.io:PrintDebug("An ENet Peer [%d] has disconnected (%s [%s])",peerID,ply:Nick(),ply:SteamID())
			else
				self.voxelate.io:PrintDebug("An ENet Peer [%d] has disconnected (Invalid player)",peerID)
			end
		else
			self.voxelate.io:PrintDebug("An unauthenticated ENet Peer [%d] has disconnected",peerID)
		end
	end)

	hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,channelID,data)
		self:IncomingPacket(peerID,data,channelID)
	end)

	hook.Add("PlayerInitialSpawn","Voxelate.PlayerConnectTimeout",function(ply)
		if not self.PUIDsEx[ply] then
			-- player hasn't synced up yet: give them 25 seconds to do so, or we boot them

			addPlayerTimeout(ply,"Voxelate.PlayerConnectTimeout",25,function()
				if not self.PeerIDsEx[ply] then
					ply:Kick("Failed to connect and authenticate with ENet (timed out)")
				end
			end)
		end
	end)

	gameevent.Listen("player_disconnect")
	hook.Add("player_disconnect","Voxelate.ManualPlayerDisconnect",function(data)
		local name = data.name
		local steamID = data.networkid
		-- local id = data.userid
		local bot = data.bot
		-- local reason = data.reason

		if bot then return end

		local ply = player.GetBySteamID(steamID)

		local peerID = self.PeerIDsEx[ply]

		if peerID then
			self.voxelate.io:PrintDebug("Disconnecting %s [%s] from ENet...",name,steamID)

			self.voxelate.networkDisconnectPeer(peerID)

			timer.Simple(5,function()
				if self.PeerIDs[peerID] then
					self.voxelate.io:PrintDebug("Forcefully disconnecting %s [%s] from ENet...",name,steamID)

					self.voxelate.networkResetPeer(peerID)

					local PUID = self.PUIDsEx[ply]

					self.PeerAddress[peerID] = nil
					self.PeerIDs[peerID] = nil
					self.PeerIDsEx[ply] = nil
					self.PeerAddress[peerID] = nil
					self.PUIDsEx[peerID] = nil
					if PUID then
						self.PUIDs[PUID] = nil
					end
				end
			end)
		end
	end)

	local function assertCompatibility(ply,serverVer,clientVer,disconnectMessage)
		local serverMajor,serverMinor,serverPatch = string.match(serverVer,"(%d+)%.(%d+)%.(%d+)")
		if not serverMajor then
			-- couldn't parse server version... what do now?
			self.voxelate.io:PrintError("Invalid server version (%s)",serverVer)
			return true
		end

		serverMajor,serverMinor,serverPatch = tonumber(serverMajor),tonumber(serverMinor),tonumber(serverPatch)

		local clientMajor,clientMinor,clientPatch = string.match(clientVer,"(%d+)%.(%d+)%.(%d+)")
		if not clientMajor then
			ply:Kick(string.format(disconnectMessage,serverVer,clientVer,"invalid version"))
			return false
		end

		clientMajor,clientMinor,clientPatch = tonumber(clientMajor),tonumber(clientMinor),tonumber(clientPatch)

		if serverMajor ~= clientMajor then
			ply:Kick(string.format(disconnectMessage,serverVer,clientVer,"major ver. mismatch"))
			return false
		end

		if serverMajor ~= clientMajor then
			ply:Kick(string.format(disconnectMessage,serverVer,clientVer,"minor ver. mismatch"))
			return false
		end

		-- good to go
		return true
	end

	util.AddNetworkString("gmod_vox_sync")

	net.Receive("gmod_vox_sync",function(len,ply)
		self.voxelate.io:PrintDebug("Network synchronisation sequence starting for %s [%s]",ply:Nick(),ply:SteamID())

		local clientModuleVersion = net.ReadString()
		local clientLuaVersion = net.ReadString()

		if not assertCompatibility(ply,self.voxelate.module.VERSION,clientModuleVersion,"gm_voxelate module version mismatch [SV (%s) != CL (%s) (%s)]") then
			self.voxelate.io:PrintDebug("Client has failed module version check (%s [%s])",ply:Nick(),ply:SteamID())
			return
		end

		if not assertCompatibility(ply,self.voxelate.LuaVersion,clientLuaVersion,"gm_voxelate Lua version mismatch [SV (%s) != CL (%s) (%s)]") then
			self.voxelate.io:PrintDebug("Client has failed Lua version check (%s [%s])",ply:Nick(),ply:SteamID())
			return
		end

		local allocatedPUID
		repeat
			allocatedPUID = self:GeneratePUID()
		until not self.PUIDs[allocatedPUID]

		self.PUIDs[allocatedPUID] = ply
		self.PUIDsEx[ply] = allocatedPUID

		self.voxelate.io:PrintDebug("Allocated PUID [%s] for %s [%s]",allocatedPUID,ply:Nick(),ply:SteamID())

		net.Start("gmod_vox_sync")
			net.WriteString(allocatedPUID)
		net.Send(ply)
	end)

	self:Listen("AuthHandshake",function(data,peerID)
		if self.PUIDs[data] then
			if self.PeerIDs[peerID] then
				self.voxelate.io:PrintError("PUID [%s] has already been allocated to a Peer ID!",data)
			else
				self.PeerIDs[peerID] = self.PUIDs[data]
				self.PeerIDsEx[self.PUIDs[data]] = peerID

				self.voxelate.io:PrintDebug("PUID [%s] has been allocated to Peer ID [%d]",data,peerID)

				removePlayerTimeout(self.PUIDs[data],"Voxelate.PlayerConnectTimeout")
			end
		else
			self.voxelate.io:PrintError("Unknown PUID [%s] cannot be allocated to Peer ID [%d]!",data,peerID)
		end
	end)
end

function Router:IncomingPacket(peerID,data,channelID)
	self:PropagateMessage(peerID,channelID,data)
end

function Router:SendInChannel(channelName,payloadData,peerID,unreliable)
	assert(type(peerID) == "number","Peer ID must be a number")
	assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

	local channelNum = self.channelsEx[channelName]

	self.voxelate.module.networkSendPacket(channelNum,payloadData,unreliable,peerID)
end

function Router:BroadcastInChannel(channelName,payloadData,unreliable)
	assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

	local channelNum = self.channelsEx[channelName]

	for peerID,_ in pairs(self.PeerIDs) do
		self.voxelate.module.networkSendPacket(channelNum,payloadData,unreliable,peerID)
	end
end
