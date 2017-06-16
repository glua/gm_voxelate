
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

	internals.networkSendPacket(channels.auth, handshakeKey)
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