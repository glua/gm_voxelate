
local internals = require("internals")
local channels = require("channels")

local TIMEOUT = 5

util.AddNetworkString("Voxelate.Auth")

local next_handshake_number = 0
local function generateHandshakeKey()
	
	local key = string.format(
		"%08x%08x%08x",
		next_handshake_number,
		math.random(2 ^ 32),
		math.random(2 ^ 32)
	)

	next_handshake_number = next_handshake_number + 1

	return key
end


local handshake_map = {}

-- PLAYERS are kicked if they do not auth soon enough.
-- HANDSHAKES are closed if they are not completed fast enough.
-- ONCE OPEN, PLAYERS are kicked and CONNECTIONS are closed if either is disconnected.

hook.Add("PlayerInitialSpawn","Voxelate.PlayerConnectTimeout",function(ply)
	print("A player connected, todo boot them if they dont handshake.")
end)

-- We wait for auth requests over source's networking.
net.Receive("Voxelate.Auth",function(len,ply)
	print("Network synchronisation sequence starting for "..ply:Nick().." ["..ply:SteamID().."]")

	-- Perform version check.
	local serverModuleVersion = internals.VERSION
	local clientModuleVersion = net.ReadString()

	if not checkCompatibility(serverModuleVersion,clientModuleVersion) then
		-- Don't need to print anything here, server prints the kick message.
		ply:Kick(string.format("Failed Voxelate version check. Server has V%s. You have V%s. Visit the Voxelate Github page for more information on version compatability.",serverModuleVersion,clientModuleVersion))
		
		return
	end

	-- Allocate a PUID and sent it back to the client.
	local handshake_key = generateHandshakeKey()
	handshake_map[handshake_key] = ply

	print("Allocated key ["..handshake_key.."] for "..ply:Nick().." ["..ply:SteamID().."]")

	net.Start("Voxelate.Auth")
		net.WriteString(handshake_key)
	net.Send(ply)
	
	-- After a timeout, delete the handshake. Booting the player is handled elsewhere.
	timer.Simple(TIMEOUT, function() handshake_map[handshake_key] = nil end )
end)

hook.Add("VoxNetworkConnect","Voxelate.Networking",function(peerID,address)
	print("New ENet peer. Todo boot if no handshake.")
end)

hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,channelID,data)
	print("New packet.")
	--self:IncomingPacket(peerID,data,channelID)
end)

--[[local runtime,exports = ...

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
					ply:Kick("Voxelate failed to establish a connection. If you do not have Voxelate, you need to install it to play on this server. You may also have an old verison of Voxelate, or it may have failed to load.")
				end
			end)
		end
	end)

	-- TODO do we really have to go through the trouble of a graceful disconnect here? The client has already disconnectd through the game, and their module has probably already been killed.
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

	local function assertCompatibility(serverVer,clientVer)
		
		-- Parse server version.
		local serverMajor,serverMinor,serverPatch = string.match(serverVer,"(%d+)%.(%d+)%.(%d+)")
		if not serverMajor then
			error("Invalid Voxelate server version (%s)",serverVer)
		end
		serverMajor,serverMinor,serverPatch = tonumber(serverMajor),tonumber(serverMinor),tonumber(serverPatch)


		-- Parse client version.
		local clientMajor,clientMinor,clientPatch = string.match(clientVer,"(%d+)%.(%d+)%.(%d+)") 
		if not clientMajor then
			return false
		end
		clientMajor,clientMinor,clientPatch = tonumber(clientMajor),tonumber(clientMinor),tonumber(clientPatch)

		-- Major versions must always match
		if serverMajor ~= clientMajor then
			return false
		end

		if serverMajor == 0 then
			-- Minor versions must match if we are in alpha.
			if serverMinor ~= clientMinor then
				return false
			end
		else
			-- Minor versions must match or be greater.
			if serverMinor > clientMinor then
				return false
			end
		end

		-- We don't really care about the patch.

		-- good to go
		return true
	end

	util.AddNetworkString("gmod_vox_sync")

	net.Receive("gmod_vox_sync",function(len,ply)
		self.voxelate.io:PrintDebug("Network synchronisation sequence starting for %s [%s]",ply:Nick(),ply:SteamID())

		-- Perform version check.
		local serverModuleVersion = self.voxelate.module.VERSION
		local clientModuleVersion = net.ReadString()

		if not assertCompatibility(serverModuleVersion,clientModuleVersion) then
			-- Don't need to print anything here, server prints the kick message.
			ply:Kick(string.format("Failed Voxelate version check. Server has V%s. You have V%s. Visit the Voxelate Github page for more information on version compatability",serverModuleVersion,clientModuleVersion))
			
			return
		end

		-- Allocate a PUID and sent it back to the client.
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
				self.PeerIDsEx[self.PUIDs[data] ] = peerID

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
]]