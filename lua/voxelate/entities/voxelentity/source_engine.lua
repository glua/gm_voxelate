local runtime,exports = ...

function exports.CreateEntity(voxelWorld,idx)
	local ent = ents.Create("voxel_entity")

	ent:SetInternalWorldIndex(voxelWorld:GetInternalIndex())
	ent:SetInternalSubEntityIndex(idx)

	return ent
end

local ENT = {}

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
