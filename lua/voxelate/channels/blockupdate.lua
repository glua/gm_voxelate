local runtime,exports = ...

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local BlockUpdateChannel = runtime.oop.create("BlockUpdateChannel")
exports.BlockUpdateChannel = BlockUpdateChannel
runtime.oop.extend(BlockUpdateChannel,NetworkChannel)

-- NOTE: ENET packet ordering is only gurenteed within the same channel, so we need to send all updates on the same channel to prevent desync.

local UPDATE_TYPE_BLOCK = 0
local UPDATE_TYPE_BULK_CUBOID = 1
local UPDATE_TYPE_BULK_SPHERE = 2

-- TODO: only send updates to a certain radius around client (for infinite worlds)
function BlockUpdateChannel:SendBlockUpdate(worldID,x,y,z,d)
	assert(SERVER,"serverside only")

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	buffer:WriteUInt(worldID,8)
	buffer:WriteInt(x,32)
	buffer:WriteInt(y,32)
	buffer:WriteInt(z,32)
	buffer:WriteUInt(d,16)
	buffer:WriteUInt(UPDATE_TYPE_BLOCK,8)

	return buffer:Broadcast()
end

function BlockUpdateChannel:SendBulkCuboidUpdate(worldID,x,y,z,sx,sy,sz,d)
	assert(SERVER,"serverside only")

	self.voxelate.io:PrintDebug("Sending cuboid update in %d...",worldID)

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	buffer:WriteUInt(worldID,8)
	buffer:WriteInt(x,32)
	buffer:WriteInt(y,32)
	buffer:WriteInt(z,32)
	buffer:WriteUInt(d,16)
	buffer:WriteUInt(UPDATE_TYPE_BULK_CUBOID,8)

	buffer:WriteInt(sx,16)
	buffer:WriteInt(sy,16)
	buffer:WriteInt(sz,16)

	return buffer:Broadcast()
end

function BlockUpdateChannel:OnIncomingPacket(packet)
	if SERVER then return end
	local buffer = packet:GetBuffer()

	local worldID = buffer:ReadUInt(8)
	local x = buffer:ReadInt(32)
	local y = buffer:ReadInt(32)
	local z = buffer:ReadInt(32)
	local blockData = buffer:ReadUInt(16)
	local type = buffer:ReadUInt(8)

	if type==UPDATE_TYPE_BLOCK then
		self.voxelate.module.voxSet(worldID,x,y,z,blockData)
	elseif type==UPDATE_TYPE_BULK_CUBOID
		local sx = buffer:ReadInt(16)
		local sy = buffer:ReadInt(16)
		local sz = buffer:ReadInt(16)

		for ix=x,x+sx do
			for iy=y,y+sy do
				for iz=z,z+sz do
					set(worldID,ix,iy,iz,blockData)
				end
			end
		end
	else
		self.voxelate.io:PrintError("Bad block update type: %d",type)
	end
end
