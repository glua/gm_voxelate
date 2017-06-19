
local channels = require("channels")

-- Start the auth sequence.
hook.Add("InitPostEntity","Voxelate.Networking.BeginAuthSequence",function()
	print("Sending auth request.")
	
	net.Start("Voxelate.Auth")
	net.WriteString(internals.VERSION)
	net.SendToServer()
end)

local handshakeKey

-- Server response. Tells us our PUID.
net.Receive("Voxelate.Auth",function(len)
	handshakeKey = net.ReadString()

	print("Starting ENet with PUID ["..handshakeKey.."]...")

	-- Connect to the server via ENet.
	local serverIP = game.GetIPAddress():gsub(":%d+$","")
	if serverIP == "loopback" then serverIP = "localhost" end

	print("Connecting to "..serverIP.."...")

	internals.networkConnect(serverIP)
end)

-- Connected via ENet. Send our PUID.
hook.Add("VoxNetworkConnect","Voxelate.Networking",function()

	print("Now connected to server.")
	local msg = string.char(channels.auth) .. handshakeKey

	internals.networkSendPacket(msg)
end)

-- Disconnet. Not really much to do.
hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function()
	print("We have disconnected from the server.")
end)

-- Configs

internals.netListeners[channels.config] = function(data)
	local configs = util.JSONToTable(data)

	for index,config in pairs(configs) do
		internals.voxNewWorld(config, index)
	end
end

-- Block updates

internals.netListeners[channels.updateBlock] = function(data)
	local buffer = internals.Reader(data)

	local worldID = buffer:ReadUInt(8)
	local x = buffer:ReadInt(32)
	local y = buffer:ReadInt(32)
	local z = buffer:ReadInt(32)
	local blockData = buffer:ReadUInt(16)

	internals.voxSet(worldID,x,y,z,blockData)
end

internals.netListeners[channels.updateRegion] = function(data)
	local buffer = internals.Reader(data)

	local worldID = buffer:ReadUInt(8)
	local x = buffer:ReadInt(32)
	local y = buffer:ReadInt(32)
	local z = buffer:ReadInt(32)
	local sx = buffer:ReadInt(32)
	local sy = buffer:ReadInt(32)
	local sz = buffer:ReadInt(32)
	local blockData = buffer:ReadUInt(16)

	internals.voxSetRegion(worldID,x,y,z,sx,sy,sz,blockData)
end

internals.netListeners[channels.updateSphere] = function(data)
	local buffer = internals.Reader(data)

	local worldID = buffer:ReadUInt(8)
	local x = buffer:ReadInt(32)
	local y = buffer:ReadInt(32)
	local z = buffer:ReadInt(32)
	local r = buffer:ReadUInt(16)
	local blockData = buffer:ReadUInt(16)

	internals.voxSetSphere(worldID,x,y,z,r,blockData)
end