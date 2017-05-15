local runtime,exports = ...

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local BulkUpdateChannel = runtime.oop.create("BulkUpdateChannel")
exports.BulkUpdateChannel = BulkUpdateChannel
runtime.oop.extend(BulkUpdateChannel,NetworkChannel)

-- TODO: only send updates to a certain radius around client (for infinite worlds)
function BulkUpdateChannel:SendCuboidUpdate(worldID,x,y,z,sx,sy,sz,d)
	assert(SERVER,"serverside only")

	self.voxelate.io:PrintDebug("Sending cuboid update in %d...",worldID)

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	buffer:WriteUInt(worldID,8)
	buffer:WriteInt(x,32)
	buffer:WriteInt(y,32)
	buffer:WriteInt(z,32)
	buffer:WriteInt(sx,16)
	buffer:WriteInt(sy,16)
	buffer:WriteInt(sz,16)
	buffer:WriteUInt(d,16)

	return buffer:Broadcast()
end

function BulkUpdateChannel:OnIncomingPacket(packet)
	if SERVER then return end
	local buffer = packet:GetBuffer()

	local worldID = buffer:ReadUInt(8)
	local x = buffer:ReadInt(32)
	local y = buffer:ReadInt(32)
	local z = buffer:ReadInt(32)
	local sx = buffer:ReadInt(16)
	local sy = buffer:ReadInt(16)
	local sz = buffer:ReadInt(16)

	local blockData = buffer:ReadUInt(16)

	local set = gm_voxelate.module.voxSet

	for ix=x,x+sx do
		for iy=y,y+sy do
			for iz=z,z+sz do
				set(worldID,ix,iy,iz,blockData)
			end
		end
	end
end
