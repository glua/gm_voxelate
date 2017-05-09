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

        self:AddVoxelWorld(index,config)

		gm_voxelate.module.voxInit(index,config)
		gm_voxelate.module.voxEnableMeshGeneration(index,true)
	else
		local index = self:GetInternalIndex()

		gm_voxelate.module.voxNew(index)

		net.Start("voxelate_req")
		net.WriteUInt(index,16)
		net.SendToServer()
	end
end
