local runtime,exports = ...

local serialization = runtime.require("../serialization")

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local VoxelWorldInitChannel = runtime.oop.create("VoxelWorldInitChannel")
exports.VoxelWorldInitChannel = VoxelWorldInitChannel
runtime.oop.extend(VoxelWorldInitChannel,NetworkChannel)

local P = {
	VOXELATE_WORLD_CONFIG = 1,
	VOXELATE_WORLD_CHUNK_STARTUP = 2,
}

function VoxelWorldInitChannel:RequestVoxelWorldConfig(worldID)
	assert(CLIENT,"clientside only")

	self.voxelate.io:PrintDebug("Requesting voxel config for %d...",worldID)

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	buffer:WriteUInt(worldID,8)
	buffer:WriteUInt(P.VOXELATE_WORLD_CONFIG,8)

	return buffer:Send()
end

function VoxelWorldInitChannel:SendVoxelWorldConfig(peerID,worldID)
	assert(SERVER,"serverside only")

	self.voxelate.io:PrintDebug("Sending voxel config for %d to %d...",worldID,peerID)

	local packet = self:NewPacket()
	packet:SetPeer(peerID)

	local buffer = packet:GetBuffer()

	local config = self.voxelate:GetWorldConfig(worldID) or {}

	buffer:WriteUInt(worldID,8)
	buffer:WriteUInt(P.VOXELATE_WORLD_CONFIG,8)

	buffer:WriteString(serialization.serialize(config))

	local res = buffer:Send()

	return res
end

function VoxelWorldInitChannel:ReceiveVoxelWorldConfig(worldID,buffer)
	assert(CLIENT,"clientside only")

	self.voxelate.io:PrintDebug("Receiving voxel config for %d...",worldID)

	local config = serialization.deserialize(buffer:ReadString())

	self.voxelate:SetWorldConfig(worldID,config)

	self.voxelate.module.voxNewWorld(config,worldID)

	self:RequestVoxelStartupChunks(worldID,0,0,0)
end

function VoxelWorldInitChannel:RequestVoxelStartupChunks(worldID,origin_x,origin_y,origin_z)
	assert(CLIENT,"clientside only")

	self.voxelate.io:PrintDebug("Requesting voxel chunk startup pack for %d...",worldID)

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	buffer:WriteUInt(worldID,8)
	buffer:WriteUInt(P.VOXELATE_WORLD_CHUNK_STARTUP,8)

	buffer:WriteUInt(origin_x,32)
	buffer:WriteUInt(origin_y,32)
	buffer:WriteUInt(origin_z,32)

	return buffer:Send()
end

function VoxelWorldInitChannel:SendVoxelStartupChunks(peerID,worldID,buffer)
	assert(SERVER,"serverside only")

	self.voxelate.io:PrintDebug("Sending voxel chunk startup pack for %d to %d...",worldID,peerID)

	-- UNUSED
	local origin_x = buffer:ReadUInt(32)
	local origin_y = buffer:ReadUInt(32)
	local origin_z = buffer:ReadUInt(32)

	local config = self.voxelate:GetWorldConfig(worldID)

	local function chunkSenderThread()
		self.voxelate.io:PrintDebug("Sending chunk initialisation data for %d to %d...",worldID,peerID)
		for x=1,math.ceil(config.dimensions.x/16) do
			for y=1,math.ceil(config.dimensions.y/16) do
				for z=1,math.ceil(config.dimensions.z/16) do
					self.voxelate.module.voxSendChunk(worldID,peerID,x,y,z)
				end

				timer.Simple(0.01,chunkSenderThread)
				coroutine.yield()
			end
		end
		self.voxelate.io:PrintDebug("Chunk initialisation data sent to %d...",peerID)
	end
	chunkSenderThread = coroutine.wrap(chunkSenderThread) -- create thread
	chunkSenderThread() -- launch thread
end

function VoxelWorldInitChannel:OnIncomingPacket(packet)
	local buffer = packet:GetBuffer()

	local worldID = buffer:ReadUInt(8)
	local requestID = buffer:ReadUInt(8)

	if SERVER then
		if requestID == P.VOXELATE_WORLD_CONFIG then
			-- time to send world config
			self:SendVoxelWorldConfig(packet:GetPeer(),worldID)
		elseif requestID == P.VOXELATE_WORLD_CHUNK_STARTUP then
			-- time to send world config
			self:SendVoxelStartupChunks(packet:GetPeer(),worldID,buffer)
		end
	else
		if requestID == P.VOXELATE_WORLD_CONFIG then
			self:ReceiveVoxelWorldConfig(worldID,buffer)
		end
	end
end
