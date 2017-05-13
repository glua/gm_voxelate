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

	hook.Add("VoxNetworkConnect","Voxelate.Networking",function(peerID,addressNum)
		local address = string.format(
			"%d.%d.%d.%d",
			bit.band(addressNum,0x000000FF),
			bit.rshift(bit.band(addressNum,0x0000FF00),8),
			bit.rshift(bit.band(addressNum,0x00FF0000),16),
			bit.rshift(bit.band(addressNum,0xFF000000),24)
		)

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
			self.voxelate.io:PrintDebug("An ENet Peer [%d] has disconnected (%s [%s])",peerID,ply:Nick(),ply:SteamID())
		else
			self.voxelate.io:PrintDebug("An unauthenticated ENet Peer [%d] has disconnected",peerID)
		end
	end)

	hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,data,channelID)
		self:IncomingPacket(peerID,data,channelID)
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

	util.AddNetworkString("gmod_vox_sync")

	net.Receive("gmod_vox_sync",function(len,ply)
		self.voxelate.io:PrintDebug("Network synchronisation sequence starting for %s [%s]",ply:Nick(),ply:SteamID())

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
				self.PeerIDs[self.PUIDs[data]] = peerID

				self.voxelate.io:PrintDebug("PUID [%s] has been allocated to Peer ID [%d]",data,peerID)
			end
		else
			self.voxelate.io:PrintError("Unknown PUID [%s] cannot be allocated to Peer ID [%d]!",data,peerID)
		end
	end)
end

function Router:IncomingPacket(peerID,data,internalChannelID)
	local reader = bitbuf.Reader(data)

	local channelID = reader:ReadUInt(16)
	local payload,ok = reader:ReadBytes(#data - 2)

	if not ok then
		error("Packet read failure")
	end

	self:PropagateMessage(peerID,channelID,payload:GetString())
end

function Router:SendInChannel(channelName,payloadData,peerID,unreliable)
	assert(type(peerID) == "number","Peer ID must be a number")
	assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

	local buf = bitbuf.Writer(#payloadData + 2)
	local payload = UCHARPTR_FromString(payloadData,#payloadData)

	buf:WriteUInt(self.channelsEx[channelName],16)
	buf:WriteBytes(payload,#payloadData)

	local data = buf:GetWrittenString()

	self.voxelate.module.networkSendPacket(data,#data,unreliable,peerID)
end

function Router:BroadcastInChannel(channelName,payloadData,unreliable)
	assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

	local buf = bitbuf.Writer(#payloadData + 2)
	local payload = UCHARPTR_FromString(payloadData,#payloadData)

	buf:WriteUInt(self.channelsEx[channelName],16)
	buf:WriteBytes(payload,#payloadData)

	local data = buf:GetWrittenString()

	for peerID,_ in pairs(self.PeerIDs) do
		self.voxelate.module.networkSendPacket(data,#data,unreliable,peerID)
	end
end
