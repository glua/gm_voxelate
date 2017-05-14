local runtime,exports = ...

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local BlockUpdateChannel = runtime.oop.create("BlockUpdateChannel")
exports.BlockUpdateChannel = BlockUpdateChannel
runtime.oop.extend(BlockUpdateChannel,NetworkChannel)

-- TODO: only send updates to a certain radius around client (for infinite worlds)
function BlockUpdateChannel:SendBlockUpdate(worldID,x,y,z)
	assert(SERVER,"serverside only")

	self.voxelate.io:PrintDebug("Sending block update [%d, %d, %d] in %d...",x,y,z,worldID)

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	local blockData = self.voxelate.module.voxGet(worldID,x,y,z)

	buffer:WriteUInt(worldID,8)
	buffer:WriteInt(x,32)
	buffer:WriteInt(y,32)
	buffer:WriteInt(z,32)
	buffer:WriteUInt(blockData,16)

	return buffer:Broadcast()
end

function VoxelWorldInitChannel:OnIncomingPacket(packet)
	local buffer = packet:GetBuffer()

	local worldID = buffer:ReadUInt(8)
	local x = buffer:ReadInt(32)
	local y = buffer:ReadInt(32)
	local z = buffer:ReadInt(32)
	local blockData = buffer:ReadUInt(16)

	self.voxelate.io:PrintDebug("Updating block [%d, %d, %d] in %d...",x,y,z,worldID)

	self.voxelate.module.voxSet(worldID,x,y,z,blockData)
end
