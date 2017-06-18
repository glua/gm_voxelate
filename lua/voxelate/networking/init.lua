

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

--[[local runtime,exports = ...

local channelHandlers = {}

internals.net = {}

internals.net.listen = function(id,handler)

	channelHandlers[id] = handler

end

internals.net.send = voxelate.module.networkSendPacket

hook.Add("Tick","Voxelate.Networking.Polling",function()
	internals.module.networkPoll()
end)

hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,channelID,payloadData)

	if channelHandlers[channelID] then
		local ply
		if peerID then
			ply = "asdf"
		end
		channelHandlers[channelID](payloadData, ply, peerID)
	else
		print("Unhandled network message in channel ID %d", channelID)
	end
end)
]]

-- move to server
--[[function Router:GeneratePUID()
	-- PUID = Pseudo-unique identifier

	return string.format(
		"%08x-%04x-%04x-%08x",
		math.random(2 ^ 32),
		math.random(2 ^ 16),
		math.random(2 ^ 16),
		math.random(2 ^ 32)
	)
end]]