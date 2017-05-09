local runtime,exports = ...

local ENT = {}

ENT.Type = "anim"

function ENT:SetupDataTables()
	self:NetworkVar("Int",0,"InternalIndex")
end

function ENT:Initialize()
	if SERVER then
		local index = gm_voxelate.module.voxNew(-1)

		self:SetInternalIndex(index)

		local config = self.config
		self.config = nil

		config._ent = self

        self:AddVoxelWorld(index,self,config)

		gm_voxelate.module.voxInit(index,config)
		gm_voxelate.module.voxEnableMeshGeneration(index,true)
	else
		local index = self:GetInternalIndex()

        self:AddVoxelWorld(index,self)

		gm_voxelate.module.voxNew(index)

		gm_voxelate.channels.voxelWorldInit:RequestVoxelWorldConfig(index)
	end
end
