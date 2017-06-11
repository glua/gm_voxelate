
local internals = require("internals")
local channels = require("channels")

local sendToServer = internals.networkSendPacket

-- Start the auth sequence.
hook.Add("InitPostEntity","Voxelate.Networking.BeginAuthSequence",function()
	print("Sending auth request.")
	
	net.Start("Voxelate.Auth")
	net.WriteString(internals.VERSION)
	net.SendToServer()
end)

local localPUID

-- Server response. Tells us our PUID.
net.Receive("Voxelate.Auth",function(len)
	localPUID = net.ReadString()

	print("Starting ENet with PUID ["..localPUID.."]...")

	-- Connect to the server via ENet.
	local serverIP = game.GetIPAddress():gsub(":%d+$","")
	if serverIP == "loopback" then serverIP = "localhost" end

	print("Connecting to "..serverIP.."...")

	internals.networkConnect(serverIP)
end)

-- Connected via ENet. Send our PUID.
hook.Add("VoxNetworkConnect","Voxelate.Networking",function()

	print("Now connected to server.")

	sendToServer(channels.auth, localPUID)
end)

-- Disconnet. Not really much to do.
hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function()
	print("We have disconnected from the server.")
end)