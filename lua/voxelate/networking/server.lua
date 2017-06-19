
local channels = require("channels")

local TIMEOUT = 5

-- Auth Shit
util.AddNetworkString("Voxelate.Auth")

local next_handshake_number = 0
local function generateHandshakeKey()
	
	local key = string.format(
		"%08x%08x%08x%08x",
		next_handshake_number,
		math.random(2 ^ 32),
		math.random(2 ^ 32),
		math.random(2 ^ 32)
	)

	next_handshake_number = next_handshake_number + 1

	return key
end

-- Maps open handshake keys to players
local handshake_map = {}

-- Maps players to peers and vice verse
local map_players_to_peers = {}
local map_peers_to_players = {}

-- Each player has a stack of chunks to be networked.
local chunk_stacks = {}

-- PLAYERS are kicked if they do not auth fast enough.
-- CONNECTIONS are closed if they do not auth fast enough.
-- HANDSHAKES are closed if they are not completed fast enough.
-- ONCE OPEN, PLAYERS and CONNECTIONS are disconnected if their matching PLAYER or CONNECTION disconnects.

hook.Add("PlayerInitialSpawn","Voxelate.PlayerConnectTimeout",function(ply)

	timer.Simple(TIMEOUT, function()
		if not map_players_to_peers[ply] and IsValid(ply) then
			
			local serverModuleVersion = internals.VERSION
			
			ply:Kick("Voxelate handshake failed. You need gm_voxelate to play on this server. Server has version "..serverModuleVersion..".")
		end
	end )

end)

hook.Add("VoxNetworkConnect","Voxelate.Networking",function(peerID,address)
	
	timer.Simple(TIMEOUT, function()
		if not map_peers_to_players[peerID] then
			internals.networkDisconnectPeer(peerID)
		end
	end )

end)

-- NEITHER of these seem to work in singleplayer tests. Need to test them better.
gameevent.Listen("player_disconnect")
hook.Add("player_disconnect","Voxelate.CleanupConnection",function(data)

	local steamID = data.networkid
	local bot = data.bot

	if bot then return end

	local ply = player.GetBySteamID(steamID) -- does this even work at this point?

	local peerID = map_players_to_peers[ply]
	print("DISCONNECT 1",ply,peerID)

	if peerID then
		print("unmapping 1")
		map_players_to_peers[ply] = nil
		map_peers_to_players[peerID] = nil
		chunk_stacks[peerID] = nil
		internals.networkDisconnectPeer(peerID)
	end
end)

hook.Add("VoxNetworkDisconnect","Voxelate.CleanupConnection",function(peerID)
	
	local ply = map_peers_to_players[peerID]
	
	print("DISCONNECT 2")

	if ply then
		print("unmapping 2")

		map_players_to_peers[ply] = nil
		map_peers_to_players[peerID] = nil
		chunk_stacks[peerID] = nil
		ply:Kick("Voxelate lost connection.")
	end
end)

local function checkCompatibility(serverVer,clientVer)
		
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


-- We wait for auth requests over source's networking.
net.Receive("Voxelate.Auth",function(len,ply)
	print("Network auth sequence starting for "..ply:Nick().." ["..ply:SteamID().."]")

	-- Perform version check.
	local serverModuleVersion = internals.VERSION
	local clientModuleVersion = net.ReadString()

	if not checkCompatibility(serverModuleVersion,clientModuleVersion) then
		-- Don't need to print anything here, server prints the kick message.
		ply:Kick(string.format("Failed Voxelate version check. Server has version %s. You have version %s. Visit the Voxelate Github page for more information on version compatability.",serverModuleVersion,clientModuleVersion))
		
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


internals.netListeners[channels.auth] = function(data, peer)
	if handshake_map[data] then
		local ply = handshake_map[data]
		handshake_map[data] = nil

		map_players_to_peers[ply] = peer
		map_peers_to_players[peer] = ply
		chunk_stacks[peer] = {}

		hook.Call("VoxNetworkAuthed", GAMEMODE, peer)
	end
end

-- Configs

internals.sendConfigs = function(configs, peers)
	if not peers then peers = map_players_to_peers end

	local msg = string.char(channels.config) .. util.TableToJSON(configs)

	for _,peer in pairs(peers) do
		internals.networkSendPacket(msg, peer)
		
		local ply = map_peers_to_players[peer]
		for worldID,config in pairs(configs) do
			chunk_stacks[peer][worldID] = internals.voxGetAllChunks(worldID, config.entity:WorldToLocal(ply:GetPos())) -- todo fix start point
		end
	end
end

-- Chunk Networking
local CHUNKS_SENT_PER_PEER_PER_TICK = 10

hook.Add("Think", "Voxelate.ChunkNetworking",function()
	for peerID,worlds in pairs(chunk_stacks) do
		
		local quota = CHUNKS_SENT_PER_PEER_PER_TICK
		for worldID,stack in pairs(worlds) do
			
			while quota > 0 and #stack > 0 do
				local chunk_pos = table.remove(stack)
				internals.voxSendChunk(worldID,peerID,chunk_pos.x,chunk_pos.y,chunk_pos.z)
				quota = quota - 1
			end
		end
	end
	--print("yo.")
	-- internals.voxSendChunk(worldID,peerID,p.x,p.y,p.z)
end)

internals.sendBlockUpdate = function(worldID, x, y, z, d)
	local writer = internals.Writer(16)

	writer:WriteUInt(channels.updateBlock,8)
	writer:WriteUInt(worldID,8)
	writer:WriteInt(x,32)
	writer:WriteInt(y,32)
	writer:WriteInt(z,32)
	writer:WriteUInt(d,16)

	local msg = writer:GetWrittenString()

	-- todo filter for yuge worlds
	for ply, peer in pairs(map_players_to_peers) do
		internals.networkSendPacket(msg, peer)
	end
end


internals.sendRegionUpdate = function(worldID, x,y,z,sx,sy,sz,d)

	local writer = internals.Writer(28)

	writer:WriteUInt(channels.updateRegion,8)
	writer:WriteUInt(worldID,8)
	writer:WriteInt(x,32)
	writer:WriteInt(y,32)
	writer:WriteInt(z,32)
	writer:WriteInt(sx,32)
	writer:WriteInt(sy,32)
	writer:WriteInt(sz,32)
	writer:WriteUInt(d,16)

	local msg = writer:GetWrittenString()

	-- todo filter for yuge worlds
	for ply, peer in pairs(map_players_to_peers) do
		internals.networkSendPacket(msg, peer)
	end
end

internals.sendSphereUpdate = function(worldID, x,y,z,r,d)

	local writer = internals.Writer(20)

	writer:WriteUInt(channels.updateSphere,8)
	writer:WriteUInt(worldID,8)
	writer:WriteInt(x,32)
	writer:WriteInt(y,32)
	writer:WriteInt(z,32)
	writer:WriteUInt(r,16)
	writer:WriteUInt(d,16)

	local msg = writer:GetWrittenString()

	-- todo filter for yuge worlds
	for ply, peer in pairs(map_players_to_peers) do
		internals.networkSendPacket(msg, peer)
	end
end

