local runtime,exports = ...

function exports.CreateEntity(voxelWorld,idx,className)
	local ent = ents.Create(className or "voxel_entity")

	if not ent.__VOXELATE__ then
		ent:Remove()
		assert("Attempted to create a non-voxelate entity")
	end

	ent:SetInternalWorldIndex(voxelWorld:GetInternalIndex())
	ent:SetInternalSubEntityIndex(idx)

	return ent
end

local ENT = {}

ENT.__VOXELATE__ = true -- simple var to check if an ent is a voxel entity
ENT.Type = "anim"

function ENT:SetupDataTables()
	self:NetworkVar("Int",0,"InternalWorldIndex")
	self:NetworkVar("Int",0,"InternalSubEntityIndex")
end

function ENT:Initialize()
	self:GetWorldEntity().subEntities[self:GetInternalSubEntityIndex()] = self
end

function ENT:OnRemove()
	self:GetWorldEntity().subEntities[self:GetInternalSubEntityIndex()] = nil
end

function ENT:GetWorldEntity()
	return gm_voxelate:GetWorldEntity(self:GetInternalWorldIndex())
end

scripted_ents.Register(ENT,"voxel_entity")
