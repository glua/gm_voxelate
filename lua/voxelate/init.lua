local runtime,exports = ...

local ClientRouter = runtime.require("./networking/client").Router
local ServerRouter = runtime.require("./networking/server").Router

local VoxelWorldInitChannel = runtime.require("./channels/voxelworldinit").VoxelWorldInitChannel
local BlockUpdateChannel = runtime.require("./channels/blockupdate").BlockUpdateChannel
--local BulkUpdateChannel = runtime.require("./channels/bulkupdate").BulkUpdateChannel

runtime.require("./entities/voxelworld")
local VoxelEntity = runtime.require("./entities/voxelentity/voxelate_engine").VoxelEntity

local IO = runtime.require("./io").IO

local Voxelate = runtime.oop.create("Voxelate")

local module = G_VOX_IMPORTS
G_VOX_IMPORTS = nil
exports.module = module

--[[Voxelate.EVoxelLoadState = { how about no?
	STALE = 0,
	SYNCHRONISING = 1,
	LOADING_CHUNKS = 2,
	READY = 3,
}]]

function Voxelate:__ctor()
	self.module = module

	--self.voxelWorldEnts = {}
	self.voxelWorldConfigs = {}

	self.channels = {}

	if CLIENT then
		self.io = IO:__new("Voxelate.CL",{r=255,g=255,b=0},{r=155,g=155,b=255})

		self.router = ClientRouter:__new(self)
	else
		self.io = IO:__new("Voxelate.SV",{r=0,g=155,b=255},{r=155,g=155,b=255})

		self.router = ServerRouter:__new(self)
	end

	self.registeredSubEntityClasses = {
		voxel_entity = VoxelEntity,
	}

	if CLIENT then
		--[[timer.Create("Voxelate.SortUpdatesOrigin",2.5,0,function()
			for index,config in pairs(self.voxelWorldConfigs) do
				if config and IsValid(config.sourceEngineEntity) then
					local scale = config.scale or 32
					local relativePos = config.sourceEngineEntity:WorldToLocal(LocalPlayer():GetPos()) / scale

					self.module.voxSortUpdatesByDistance(index,relativePos)
				end
			end
		end)]]
	end

	--[[hook.Add("Tick","Voxelate.TrackWorldUpdates",function()
		for worldID,_ in pairs(self.voxelWorldConfigs) do
			self.module.voxGetWorldUpdates(worldID)
		end
	end)

	hook.Add("VoxWorldBlockUpdate","Voxelate.TrackWorldUpdates",function(worldID,x,y,z)
		local ent = self:GetWorldEntity(worldID)

		if ent then
			if SERVER then
				self.channels.blockUpdate:SendBlockUpdate(worldID,x,y,z)
			end

			if ent.OnBlockUpdate then
				ent:OnBlockUpdate(x,y,z)
			end
		end
	end)]]

	self:AddChannel(VoxelWorldInitChannel,"voxelWorldInit",2)
	self:AddChannel(BlockUpdateChannel,"blockUpdate",3)
	--self:AddChannel(BulkUpdateChannel,"bulkUpdate",4)
end

function Voxelate:AddChannel(class,name,id)
	self.channels[name] = class:__new(name,id)
	self.channels[name]:BindToRouter(self.router)
end

-- i dont think this is really necessary
--function Voxelate:AddVoxelWorld(index,ent,config)
--	self.voxelWorldEnts[index] = ent
--	self.voxelWorldConfigs[index] = config
--end

function Voxelate:GetWorldEntity(index)
	return self:GetWorldConfig(index).sourceEngineEntity
end

function Voxelate:GetWorldConfig(index)
	return self.voxelWorldConfigs[index]
end

-- warning - this might get called without a valid ent in the future - is this an issue?
function Voxelate:SetWorldConfig(index,config)
	self.voxelWorldConfigs[index] = config
end

function Voxelate:RegisterSubEntity(className,classObj)
	assert(some_condition_to_be_decided,"For voxelate-engine based VoxelWorld entities only")

	runtime.oop.extend(classObj,VoxelEntity)

	self.registeredSubEntityClasses[className] = classObj
end

function Voxelate:ResolveSubEntityClassName(className)
	assert(self.registeredSubEntityClasses[className],"Unknown voxel entity class name")
	return self.registeredSubEntityClasses[className]
end

exports.Voxelate = Voxelate:__new()

-- Don't let players screw with our voxels!
hook.Add("PhysgunPickup", "Voxelate.NoPhysgun", function(ply,ent)
	if ent:GetClass() == "voxel_world" then return false end
end)

hook.Add("CanTool", "Voxelate.NoTool", function(ply,tr,tool)
	if IsValid( tr.Entity ) and tr.Entity:GetClass() == "voxel_world" then return false end
end)
