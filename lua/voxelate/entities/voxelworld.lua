local runtime,exports = ...

local ENT = {}

ENT.Type = "anim"

function ENT:SetupDataTables()
	self:NetworkVar("Int",0,"InternalIndex")
end

function ENT:Initialize()
	if SERVER then
		local config = self.config
		self.config = nil

		config._ent = self

		local index = gm_voxelate.module.voxNewWorld(config)

		self:SetInternalIndex(index)

		gm_voxelate:SetWorldConfig(index,config)

		self:SetupBounds(config)

		if config.generator then
			self:generate(config.generator)
		else
			self:generateDefault()
		end
	else
		local index = self:GetInternalIndex()

		gm_voxelate.channels.voxelWorldInit:RequestVoxelWorldConfig(index)
	end
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
	gm_voxelate.module.voxUpdate(index,10,self)

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
end


scripted_ents.Register(ENT,"voxel_world")