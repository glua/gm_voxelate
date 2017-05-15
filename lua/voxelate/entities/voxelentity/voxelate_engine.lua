local runtime,exports = ...

local VoxelEntity = runtime.oop.create("VoxelEntity")
exports.VoxelEntity = VoxelEntity

-- todo: network worldID, and set up basic networking

function exports.CreateEntity(voxelWorld,idx,classObj)
	classObj = classObj or VoxelEntity

	assert(runtime.oop.inherits(classObj,VoxelEntity),"Attempted to create a non-voxelate entity")

	return classObj:__new(voxelWorld,idx)
end

function VoxelEntity:__ctor(voxelWorld,idx)
	self.worldID = voxelWorld:GetInternalIndex()
	self.subEntityID = idx

	self:GetWorldEntity().subEntities[self.subEntityID] = self
end

function VoxelEntity:__dtor()
	self:GetWorldEntity().subEntities[self.subEntityID] = nil
end

function VoxelEntity:GetWorldEntity()
	return gm_voxelate:GetWorldEntity(self.worldID)
end
