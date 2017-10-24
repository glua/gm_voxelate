
local ENT = {}

ENT.Type = "anim"

local configs = {}

hook.Add("VoxNetworkAuthed","Voxelate.SendConfigs",function(peerID)
	internals.sendConfigs(configs, {peerID})
end)

function ENT:SetupDataTables()
	self:NetworkVar("Int",0,"InternalIndex")
end

function ENT:Initialize()
	--self.subEntities = {}
	--self.incrementingSubEntityIndex = 0

	hook.Add("PostDrawTranslucentRenderables","voxl",function()
		self:Draw()
	end)

	if SERVER then
		--self:UpdateVoxelLoadState("LOADING_CHUNKS")

		local config = self.config
		self.config = nil

		config.entity = self
		config.entityID = self:EntIndex()

		local index = internals.voxNewWorld(config)
		--config.index = index
		internals.setSourceWorldPos(index, self:GetPos())
		
		configs[index] = config

		internals.sendConfigs({[index] = config})

		self:SetInternalIndex(index)

		self:SetupBounds()

		--[[if config.generator then
			self:generate(config.generator)
		else
			self:generateDefault()
		end]]

		--gm_voxelate.module.voxSetWorldUpdatesEnabled(index,true)
		--self:UpdateVoxelLoadState("READY")
	end
end

function ENT:OnRemove()
	if not self:isInternalsReady() then return end
	local index = self:GetInternalIndex()

	internals.voxDeleteWorld(index)

	configs[index] = nil
end

function ENT:SetupBounds()
	if not self:isInternalsReady() then return end
	local index = self:GetInternalIndex()

	local mins,maxs = internals.voxBounds(index)

	-- mins = mins + self:GetPos()
	-- maxs = maxs + self:GetPos()

	self:EnableCustomCollisions(true)
	self:SetSolid(SOLID_BBOX)

	-- local mins = Vector(0,0,0)
	-- local maxs = bounds

	self.correct_maxs = maxs

	self:SetCollisionBounds(mins,maxs)
	if CLIENT then
		self:SetRenderBounds(mins,maxs)
	end
end

function ENT:Think()
	if not self:isInternalsReady() then return end
	local index = self:GetInternalIndex()

	-- 100 updates per frame is -SERIOUSLY- excessive
	-- the entire point of queueing updates is so we dont lag balls
	internals.voxUpdate(index,20,self)

	self:SetupBounds()
	
	self:NextThink(CurTime())
	return true
end

function ENT:Draw()
	if not self:isInternalsReady() then return end
	local index = self:GetInternalIndex()

	local m = Matrix()
	m:Translate(self:GetPos())

	cam.PushModelMatrix(m)
	internals.voxDraw(index)
	cam.PopModelMatrix()
end

function ENT:SetPos(pos)
	if self:isInternalsReady() then
		local index = self:GetInternalIndex()
		internals.setSourceWorldPos(index,pos)
	end

	debug.getregistry().Entity.SetPos(self,pos)
end

function ENT:isInternalsReady()
	if SERVER then return true end

	return internals.voxReady(self:GetInternalIndex())
end

function ENT:TestCollision(start,delta,isbox,extents)
	if not self:isInternalsReady() then return end

	local index = self:GetInternalIndex()

	start = internals.voxSourceWorldToBlockPos(index,start)
	delta = AdvancedVectorFromSource(delta)
	if extents then
		extents = AdvancedVectorFromSource(extents)
	end

	local fraction,hitpos,normal = internals.voxTrace(index,start,delta,isbox,extents)

	if fraction then
		hitpos = internals.voxBlockPosToSourceWorld(hitpos)

		return {
			Fraction = fraction,
			HitPos = hitpos,
			Normal = normal:toSourceVector(),
		}
	end
end

--[[function ENT:CreateSubEntity(className)
	-- negative indexes are clientside-created ents
	-- positive indexes are serverside-created ents

	if CLIENT then
		self.incrementingSubEntityIndex = self.incrementingSubEntityIndex - 1
	else
		self.incrementingSubEntityIndex = self.incrementingSubEntityIndex + 1
	end

	local index = self.incrementingSubEntityIndex

	if some_condition_to_be_decided then
		return CreateVoxelateEngineSubEntity(self,index,gm_voxelate:ResolveSubEntityClassName(className)) -- object used
	else
		return CreateSourceEngineSubEntity(self,index,className) -- name used
	end
end]]

if SERVER then
	--[[function ENT:generate(f)
		local index = self:GetInternalIndex()
		gm_voxelate.module.voxGenerate(index,f)
		--gm_voxelate.module.voxReInit(index)
		--gm_voxelate.module.voxFlagAllChunksForUpdate(index)
	end

	function ENT:generateDefault()
		local index = self:GetInternalIndex()
		gm_voxelate.module.voxGenerateDefault(index)
		--gm_voxelate.module.voxReInit(index)
		--gm_voxelate.module.voxFlagAllChunksForUpdate(index)
	end]]

	function ENT:getBlock(x,y,z)
		local index = self:GetInternalIndex()
		return internals.voxGet(index,x,y,z)
	end


	function ENT:getAt(pos)
		local index = self:GetInternalIndex()
		local scale = internals.voxGetBlockScale(index)

		local rel_pos = self:WorldToLocal(pos) / scale
		return self:getBlock(
			internals.preciseToNormalCoords(rel_pos.x),
			internals.preciseToNormalCoords(rel_pos.y),
			internals.preciseToNormalCoords(rel_pos.z)
		)
	end

	if SERVER then
		function ENT:setBlock(x,y,z,d)
			local index = self:GetInternalIndex()

			local success = internals.voxSet(index,x,y,z,d)
			if success then internals.sendBlockUpdate(index,x,y,z,d) end
			return success
		end

		function ENT:setAt(pos,d)
			local index = self:GetInternalIndex()
			local scale = internals.voxGetBlockScale(index)


			local rel_pos = self:WorldToLocal(pos) / scale

			self:setBlock(
				internals.preciseToNormalCoords(rel_pos.x),
				internals.preciseToNormalCoords(rel_pos.y),
				internals.preciseToNormalCoords(rel_pos.z),
				d
			)
		end

		function ENT:setRegion(x,y,z,sx,sy,sz,d)
			local index = self:GetInternalIndex()

			local success = internals.voxSetRegion(index,x,y,z,sx,sy,sz,d)
			if success then internals.sendRegionUpdate(index,x,y,z,sx,sy,sz,d) end

			return success
		end

		function ENT:setRegionAt(v1,v2,d)
			local index = self:GetInternalIndex()
			local scale = internals.voxGetBlockScale(index)

			local lower=self:WorldToLocal(v1) / scale
			local upper=self:WorldToLocal(v2) / scale

			OrderVectors(lower,upper)

			local fix = math.floor

			self:setRegion(
				internals.preciseToNormalCoords(lower.x),
				internals.preciseToNormalCoords(lower.y),
				internals.preciseToNormalCoords(lower.z),
				fix(upper.x)
				-fix(lower.x),
				fix(upper.y) - fix(lower.y),
				fix(upper.z) - fix(lower.z),
				d
			)
		end

		function ENT:setSphere(x,y,z,r,d)
			local index = self:GetInternalIndex()

			local success = internals.voxSetSphere(index,x,y,z,r,d)
			if success then internals.sendSphereUpdate(index,x,y,z,r,d) end
			return success
		end

		function ENT:setSphereAt(pos,r,d)
			local index = self:GetInternalIndex()
			local scale = internals.voxGetBlockScale(index)

			pos=self:WorldToLocal(pos) / scale
			r = r / scale

			self:setSphere(
				internals.preciseToNormalCoords(pos.x),
				internals.preciseToNormalCoords(pos.y),
				internals.preciseToNormalCoords(pos.z),
				r,
				d
			)
		end
	end

	function ENT:save(file_name)
		local serialized = gm_voxelate.module.voxSaveToString1(self:GetInternalIndex())

		if serialized then
			file.Write(file_name,serialized)
			return true
		end
	end

	function ENT:load(file_name)
		local f = file.Open(file_name,"rb","DATA")
		if not f then return false end

		local serialized = f:Read(f:Size())
		f:Close()

		if not serialized then return false end

		return gm_voxelate.module.voxLoadFromString1(self:GetInternalIndex(),serialized)
	end
end

function ENT:UpdateTransmitState()
	return TRANSMIT_ALWAYS
end

scripted_ents.Register(ENT,"voxel_world")
