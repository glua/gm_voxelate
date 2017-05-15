local runtime,exports = ...

local CreateSourceEngineSubEntity = runtime.require("./voxelentity/source_engine").CreateEntity
local CreateVoxelateEngineSubEntity = runtime.require("./voxelentity/voxelate_engine").CreateEntity

local ENT = {}

ENT.Type = "anim"

function ENT:SetupDataTables()
	self:NetworkVar("Int",0,"InternalIndex")
end

-- Internal; Do not call (how about never call, not even internally?)
--[[function ENT:UpdateVoxelLoadState(state,progress)
	assert(gm_voxelate.EVoxelLoadState[state],"Unknown index passed into ENT:UpdateVoxelLoadState()")

	self.state = {
		enum = gm_voxelate.EVoxelLoadState[state],
		progress = progress or 1,
	}
end

function ENT:IsReady() -- its exactly this kind of shit i was trying to avoid when i removed like half of the api
	return self.state and (self.state.enum == gm_voxelate.EVoxelLoadState.READY)
end]]

function ENT:Initialize()
	self.subEntities = {}
	self.incrementingSubEntityIndex = 0

	if SERVER then
		--self:UpdateVoxelLoadState("LOADING_CHUNKS")

		local config = self.config
		self.config = nil

		config.sourceEngineEntity = self

		local index = gm_voxelate.module.voxNewWorld(config)

		self:SetInternalIndex(index)

		gm_voxelate:SetWorldConfig(index,config)

		self:SetupBounds(config)

		if config.generator then
			self:generate(config.generator)
		else
			self:generateDefault()
		end

		--gm_voxelate.module.voxSetWorldUpdatesEnabled(index,true)
		--self:UpdateVoxelLoadState("READY")
	else
		--self:UpdateVoxelLoadState("SYNCHRONISING")

		local index = self:GetInternalIndex()

		gm_voxelate.channels.voxelWorldInit:RequestVoxelWorldConfig(index)
	end
end

function ENT:OnRemove()
	local index = self:GetInternalIndex()

	gm_voxelate.module.voxDeleteWorld(index)
end

function ENT:GetConfig()
	return gm_voxelate:GetWorldConfig(self:GetInternalIndex())
end

-- Called by module... We can probably just call it directly though.
function ENT:SetupBounds(config)

	self:EnableCustomCollisions(true)
	self:SetSolid(SOLID_BBOX)

	local dims = config.dimensions or Vector(16,16,16)
	local scale = config.scale or 32

	local mins = Vector(0,0,0)
	local maxs = dims*scale

	self.correct_maxs = maxs

	self:SetCollisionBounds(mins,maxs)
	if CLIENT then
		self:SetRenderBounds(mins,maxs)
	end
end

function ENT:Think()
	local index = self:GetInternalIndex()

	gm_voxelate.module.voxUpdate(index,CLIENT and 100 or 25,self)

	if CLIENT then
		if not self.correct_maxs then
			-- bounds not setup, try setting them up.
			local config = gm_voxelate:GetWorldConfig(index)
			if config then
				self:SetupBounds(config)
			end
		else
			-- bounds are set up, see if they need fixed.
			local _,maxs = self:GetRenderBounds()
			if maxs~=self.correct_maxs then
				self:SetRenderBounds(Vector(),self.correct_maxs)
				print("Corrected render bounds on Voxel System #"..index..".")
			end
		end
	end

	self:NextThink(CurTime())
	return true
end

function ENT:Draw()
	local index = self:GetInternalIndex()

	local m = Matrix()
	m:Translate(self:GetPos())

	-- TODO: when in an infinite world, place the player at Vec(0,0,0) and render the voxel world around the player

	cam.PushModelMatrix(m)
	gm_voxelate.module.voxDraw(index)
	cam.PopModelMatrix()
end

function ENT:TestCollision(start,delta,isbox,extents)
	if isbox then
		--debugoverlay.Box(start,Vector(-extents.x,-extents.y,0),Vector(extents.x,extents.y,extents.z*2),.05,Color(255,255,0,0))
	end
	start=self:WorldToLocal(start)
	local index = self:GetInternalIndex()
	local fraction,hitpos,normal = gm_voxelate.module.voxTrace(index,start,delta,isbox,extents)
	if fraction then
		hitpos = self:LocalToWorld(hitpos)
		if isbox and (normal.x~=0 or normal.y~=0) then
			--debugoverlay.Box(hitpos,Vector(-extents.x,-extents.y,0),Vector(extents.x,extents.y,extents.z*2),.05,Color(255,0,0,0))
			--debugoverlay.Line(hitpos,hitpos+normal*100,.05,Color(0,0,255))
		end
		return {
			Fraction = fraction,
			HitPos=hitpos,
			Normal=normal
		}
	end
end

function ENT:CreateSubEntity(className)
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
end

if SERVER then
	function ENT:generate(f)
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
	end

	function ENT:getBlock(x,y,z)
		local index = self:GetInternalIndex()
		return gm_voxelate.module.voxGet(index,x,y,z)
	end

	function ENT:setBlock(x,y,z,d)
		local index = self:GetInternalIndex()

		local success = gm_voxelate.module.voxSet(index,x,y,z,d)
		if success then gm_voxelate.channels.blockUpdate:SendBlockUpdate(index,x,y,z,d) end
		return success
	end

	function ENT:getAt(pos)
		local scale = self:GetConfig().scale or 32

		local rel_pos = self:WorldToLocal(pos)/scale
		return self:getBlock(rel_pos.x,rel_pos.y,rel_pos.z)
	end

	function ENT:setAt(pos,d)
		local scale = self:GetConfig().scale or 32

		local rel_pos = self:WorldToLocal(pos)/scale
		self:setBlock(rel_pos.x,rel_pos.y,rel_pos.z,d)
	end

	-- TODO: implement voxSetRegion in C++ so we can network it separately/faster/efficiently
	function ENT:setRegion(x,y,z,sx,sy,sz,d) --allow d to be string data
		local index = self:GetInternalIndex()
		local set = gm_voxelate.module.voxSet

		local fix = math.floor

		x = fix(x)
		y = fix(y)
		z = fix(z)
		sx = fix(sx)
		sy = fix(sy)
		sz = fix(sz)

		local success = false

		for ix=x,x+sx do
			for iy=y,y+sy do
				for iz=z,z+sz do
					success = set(index,ix,iy,iz,d) or success
				end
			end
		end

		if success then gm_voxelate.channels.blockUpdate:SendBulkCuboidUpdate(index,x,y,z,sx,sy,sz,d) end
		return success
	end

	function ENT:setRegionAt(v1,v2,d)
		local scale = self:GetConfig().scale or 32

		local lower=self:WorldToLocal(v1)/scale
		local upper=self:WorldToLocal(v2)/scale

		OrderVectors(lower,upper)

		local fix = math.floor

		self:setRegion(lower.x,lower.y,lower.z,fix(upper.x)-fix(lower.x),fix(upper.y)-fix(lower.y),fix(upper.z)-fix(lower.z),d)
	end

	function ENT:setSphere(x,y,z,r,d)
		local index = self:GetInternalIndex()
		local set = gm_voxelate.module.voxSet

		local fix = math.floor

		x = fix(x)
		y = fix(y)
		z = fix(z)
		r = fix(r)

		local success = false

		local rsqr = r*r
		for ix=x-r,x+r do
			local xsqr = (ix-x)*(ix-x)
			for iy=y-r,y+r do
				local xysqr = xsqr+(iy-y)*(iy-y)
				for iz=z-r,z+r do
					local xyzsqr = xysqr+(iz-z)*(iz-z)
					if xyzsqr<=rsqr then
						success = set(index,ix,iy,iz,d) or success
					end
				end
			end
		end

		if success then gm_voxelate.channels.blockUpdate:SendBulkSphereUpdate(index,x,y,z,r,d) end
		return success
	end

	function ENT:setSphereAt(pos,r,d)
		local scale = self:GetConfig().scale or 32

		pos=self:WorldToLocal(pos)/scale
		r=r/scale

		self:setSphere(pos.x,pos.y,pos.z,r,d)
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
