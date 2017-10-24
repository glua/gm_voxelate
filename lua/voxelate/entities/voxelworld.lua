
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

		local index = internals.voxNewWorld(config)
		--config.index = index

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
	local index = self:GetInternalIndex()

	internals.voxDeleteWorld(index)

	configs[index] = nil
end

function ENT:SetupBounds()
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
	local index = self:GetInternalIndex()

	-- 100 updates per frame is -SERIOUSLY- excessive
	-- the entire point of queueing updates is so we dont lag balls
	internals.voxUpdate(index,20,self)

	if CLIENT then
		if self.correct_maxs then
			-- bounds are set up, see if they need fixed.
			local _,maxs = self:GetRenderBounds()
			if maxs~=self.correct_maxs then
				self:SetRenderBounds(Vector(),self.correct_maxs)
				print("Corrected render bounds on Voxel System #"..index..".")
			end
		end
	end

	self:SetupBounds()
	
	self:NextThink(CurTime())
	return true
end

-- local _self,chunks
function ENT:Draw()
	-- _self = self
	local index = self:GetInternalIndex()

	local m = Matrix()
	m:Translate(self:GetPos())

	-- TODO: when in an infinite world, place the player at Vec(0,0,0) and render the voxel world around the player

	cam.PushModelMatrix(m)
	internals.voxDraw(index)
	cam.PopModelMatrix()

	-- chunks = chunks or {}

	-- for i,pos in ipairs(chunks) do
	-- -- for i=1,9 do
	-- 	-- local pos = chunks[i]
	-- 	-- if not chunks[i] then break end
	-- 	render.DrawWireframeBox(
	-- 		pos * 16 * 40 + self:GetPos(),
	-- 		Angle(0),
	-- 		Vector(0),
	-- 		Vector(16 * 40,16 * 40,16 * 40),
	-- 		Color(255,0,0),
	-- 		false
	-- 	)
	-- end
end

-- timer.Create("Voxelate.DebugMesh",1,0,function()
-- 	if not _self then return end
-- 	local player = LocalPlayer():GetPos()
-- 	local center = _self:WorldToLocal(LocalPlayer():GetPos())
-- 	-- chunks = internals.voxGetAllChunks(_self:GetInternalIndex(),center)
-- 	-- print(center / 40 / 16)
-- 	-- table.sort(chunks,function(c1,c2)
-- 	-- 	local c1pos = c1 * 16 * 40 + _self:GetPos()
-- 	-- 	local c2pos = c2 * 16 * 40 + _self:GetPos()
-- 	-- 	return c1pos:LengthSqr(player) < c2pos:LengthSqr(player)
-- 	-- end)
-- 	-- print(chunks[1],player)

	-- print("FRAMEDUMP")

	-- print(center / 40)

	-- center = center / 40 / 16
	-- center = Vector(math.floor(center.x),math.floor(center.y),math.floor(center.z))

	-- print(center)
	-- print(pcall(function()
	-- 	internals.loadChunk(_self:GetInternalIndex(),{center.x,center.y,center.z})
	-- end))

	-- chunks = {
-- 		center + Vector(-1,-1,-1),
-- 		center + Vector(-1,-1,0),
-- 		center + Vector(-1,-1,1),
-- 		center + Vector(-1,0,-1),
-- 		center + Vector(-1,0,0),
-- 		center + Vector(-1,0,1),
-- 		center + Vector(-1,1,-1),
-- 		center + Vector(-1,1,0),
-- 		center + Vector(-1,1,1),
		
-- 		center + Vector(0,-1,-1),
-- 		center + Vector(0,-1,0),
-- 		center + Vector(0,-1,1),
-- 		center + Vector(0,0,-1),
		-- center,
-- 		center + Vector(0,0,1),
-- 		center + Vector(0,1,-1),
-- 		center + Vector(0,1,0),
-- 		center + Vector(0,1,1),

-- 		center + Vector(1,-1,-1),
-- 		center + Vector(1,-1,0),
-- 		center + Vector(1,-1,1),
-- 		center + Vector(1,0,-1),
-- 		center + Vector(1,0,0),
-- 		center + Vector(1,0,1),
-- 		center + Vector(1,1,-1),
-- 		center + Vector(1,1,0),
-- 		center + Vector(1,1,1),
	-- }

-- 	-- for x=1,16 do
-- 	-- 	for y=1,16 do
-- 	-- 		for z=1,16 do
-- 	-- 			chunks[#chunks + 1] = center + Vector(x-1,y-1,z-1)
-- 	-- 		end
-- 	-- 	end
-- 	-- end
-- end)

function ENT:TestCollision(start,delta,isbox,extents)
	if isbox then
		--debugoverlay.Box(start,Vector(-extents.x,-extents.y,0),Vector(extents.x,extents.y,extents.z*2),.05,Color(255,255,0,0))
	end
	start=self:WorldToLocal(start)
	local index = self:GetInternalIndex()
	local fraction,hitpos,normal = internals.voxTrace(index,start,delta,isbox,extents)
	if fraction then
		hitpos = self:LocalToWorld(hitpos)
		if isbox and (normal.x~=0 or normal.y~=0) then
			-- debugoverlay.Box(hitpos,Vector(-extents.x,-extents.y,0),Vector(extents.x,extents.y,extents.z*2),.05,Color(255,0,0,0))
			-- debugoverlay.Line(hitpos,hitpos+normal*100,.05,Color(0,0,255))
		end
		return {
			Fraction = fraction,
			HitPos=hitpos,
			Normal=normal
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
