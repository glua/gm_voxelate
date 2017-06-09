local runtime,exports = ...

runtime.require("./shared")

-- Start the sync sequence.
hook.Add("InitPostEntity","Voxelate.Networking.BeginSyncSequence",function()
	net.Start("gmod_vox_sync")
	net.WriteString(internals.module.VERSION)
	net.SendToServer()
end)

local localPUID

-- Server response. Tells us our PUID.
net.Receive("gmod_vox_sync",function(len)
	localPUID = net.ReadString()

	gm_voxelate.io:PrintDebug("Starting ENet with PUID [%s]...",localPUID)

	-- Connect to the server via ENet.
	local serverIP = game.GetIPAddress():gsub(":%d+$","")
	if serverIP == "loopback" then serverIP = "localhost" end

	gm_voxelate.io:PrintDebug("Connecting to %s...",serverIP)

	internals.module.networkConnect(serverIP)
end)

-- Connected via ENet. Send our PUID.
hook.Add("VoxNetworkConnect","Voxelate.Networking",function()

	gm_voxelate.io:PrintDebug("Now connected to server.")

	internals.net.sendAuth(localPUID)
end)

hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function()
	gm_voxelate.io:PrintDebug("We have disconnected from the server...")
end)

--[[function Router:IncomingPacket(data,channelID)
	self:PropagateMessage(false,channelID,data)
end

function Router:SendInChannel(channelName,payloadData,unreliable)
	assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

	if self.ready then
		self.voxelate.module.networkSendPacket(self.channelsEx[channelName],payloadData,unreliable)
	else
		self.packetsToSendOnReady[#self.packetsToSendOnReady + 1] = {
			data = payloadData,
			unreliable = unreliable,
			channel = self.channelsEx[channelName],
		}
	end
end
]]