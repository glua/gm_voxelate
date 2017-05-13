local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

local SharedRouter = runtime.require("./shared").Router
runtime.oop.extend(Router,SharedRouter)

local bitbuf = runtime.require("../bitbuffer")

function Router:__ctor(voxelate)
	self.super:__ctor(voxelate)
	self.ready = false
	self.packetsToSendOnReady = {}

	hook.Add("InitPostEntity","Voxelate.Networking.BeginSyncSequence",function()
		net.Start("gmod_vox_sync")
		net.SendToServer()
	end)

	hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function()
		self.voxelate.io:PrintDebug("We have disconnected from the server...")
	end)

	hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,channelID,data)
		self:IncomingPacket(data,channelID)
	end)

	net.Receive("gmod_vox_sync",function(len)
		local PUID = net.ReadString()

		self.clientPUID = PUID

		self:StartENet()
	end)
end

function Router:StartENet()
	self.voxelate.io:PrintDebug("Starting ENet with PUID [%s]...",self.clientPUID)

	local serverIP = game.GetIPAddress():gsub(":%d+$","")
	if serverIP == "loopback" then serverIP = "localhost" end

	self.voxelate.io:PrintDebug("Connecting to %s...",serverIP)

	local suc,err = self.voxelate.module.networkConnect(serverIP)

	if suc then
		self.voxelate.io:Print("Now connected to %s via ENet!",serverIP)

		self:SendInChannel("AuthHandshake",self.clientPUID)
		self.ready = true

		for i,data in ipairs(self.packetsToSendOnReady) do
			self.voxelate.module.networkSendPacket(data.channel,data.data,#data.data,data.unreliable)
		end

		self.packetsToSendOnReady = {}
	else
		self.voxelate.io:PrintError("Couldn't connect to %s via ENet: %s",serverIP,tostring(err))
	end
end

function Router:IncomingPacket(data,channelID)
	self:PropagateMessage(false,channelID,data)
end

function Router:SendInChannel(channelName,payloadData,unreliable)
	assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

	if self.ready then
		self.voxelate.module.networkSendPacket(self.channelsEx[channelName],payloadData,#payloadData,unreliable)
	else
		self.packetsToSendOnReady[#self.packetsToSendOnReady + 1] = {
			data = payloadData,
			unreliable = unreliable,
			channel = self.channelsEx[channelName],
		}
	end
end
