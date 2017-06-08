local runtime,exports = ...

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local BlockUpdateChannel = runtime.oop.create("BlockUpdateChannel")
exports.BlockUpdateChannel = BlockUpdateChannel
runtime.oop.extend(BlockUpdateChannel,NetworkChannel)

-- NOTE: ENET packet ordering is only gurenteed within the same channel, so we need to send all updates on the same channel to prevent desync.
-- Much less likely to be an issue, but chunk updates may need to be on the same channel as well.
-- It may suit us better to just use a single channel and add our own system underneath.

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

	return buffer:Broadcast() -- todo only send to nearby players
end

function BlockUpdateChannel:SendBulkCuboidUpdate(worldID,x,y,z,sx,sy,sz,d)
	assert(SERVER,"serverside only")

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

	return buffer:Broadcast() -- todo only send to nearby players
end

function BlockUpdateChannel:SendBulkSphereUpdate(worldID,x,y,z,r,d)
	assert(SERVER,"serverside only")

	local packet = self:NewPacket()

	local buffer = packet:GetBuffer()

	buffer:WriteUInt(worldID,8)
	buffer:WriteInt(x,32)
	buffer:WriteInt(y,32)
	buffer:WriteInt(z,32)
	buffer:WriteUInt(d,16)
	buffer:WriteUInt(UPDATE_TYPE_BULK_SPHERE,8)

	buffer:WriteUInt(r,16)

	return buffer:Broadcast() -- todo only send to nearby players
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

	local set = self.voxelate.module.voxSet

	if type==UPDATE_TYPE_BLOCK then
		set(worldID,x,y,z,blockData)
	elseif type==UPDATE_TYPE_BULK_CUBOID then
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
	elseif type==UPDATE_TYPE_BULK_SPHERE then
		local r = buffer:ReadUInt(16)

		local rsqr = r*r
		for ix=x-r,x+r do
			local xsqr = (ix-x)*(ix-x)
			for iy=y-r,y+r do
				local xysqr = xsqr+(iy-y)*(iy-y)
				for iz=z-r,z+r do
					local xyzsqr = xysqr+(iz-z)*(iz-z)
					if xyzsqr<=rsqr then
						set(index,ix,iy,iz,blockData)
					end
				end
			end
		end
	else
		self.voxelate.io:PrintError("Bad block update type: %d",type)
	end
end
