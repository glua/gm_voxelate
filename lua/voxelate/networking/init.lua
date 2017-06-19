

internals.netListeners = {}


hook.Add("Tick","Voxelate.Networking.Polling",function()
	internals.networkPoll()
end)

hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,channelID,payloadData)

	if internals.netListeners[channelID] then
		internals.netListeners[channelID](payloadData, peerID)
	else
		print("Unhandled network message in channel ID ".. channelID)
	end
end)

require("bitbuffer")

if SERVER then
	require("server")
else
	require("client")
end
