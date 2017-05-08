local IMPORTS = gm_voxelate.IMPORTS
local CONFIGS = gm_voxelate.CONFIGS

local ENT = {}

ENT.Type = "anim"

function ENT:SetupDataTables()
	self:NetworkVar("Int",0,"InternalIndex")
end

function ENT:Initialize()
	if SERVER then
		local index = IMPORTS.voxNew(-1)

		self:SetInternalIndex(index)

		local config = self.config
		self.config = nil

		config._ent = self

		CONFIGS[index] = config

		IMPORTS.voxInit(index,config)
		IMPORTS.voxEnableMeshGeneration(index,true)
	else
		local index = self:GetInternalIndex()

		IMPORTS.voxNew(index)

		net.Start("voxelate_req")
		net.WriteUInt(index,16)
		net.SendToServer()
	end
end

function ENT:OnRemove()
	if SERVER then
		local index = self:GetInternalIndex()

		if SERVER then
			CONFIGS[index] = nil
		end

		IMPORTS.voxDelete(index)

		net.Start("voxelate_explicit_delete")
		net.WriteUInt(index,16)
		net.Broadcast()
	end
end

function ENT:_initMisc(size_x,size_y,size_z)
	self:EnableCustomCollisions(true)
	self:SetSolid(SOLID_BBOX)

	local mins = Vector(0,0,0)
	local maxs = Vector(size_x,size_y,size_z)

	self.correct_maxs = maxs

	self:SetCollisionBounds(mins,maxs)
	if CLIENT then
		self:SetRenderBounds(mins,maxs)
	end
end

function ENT:Think()
	local index = self:GetInternalIndex()
	IMPORTS.voxUpdate(index,10,self)

	if CLIENT and self.correct_maxs then
		local _,maxs = self:GetRenderBounds()
		if maxs~=self.correct_maxs then
			self:SetRenderBounds(Vector(),self.correct_maxs)
			print("Corrected render bounds on Voxel System #"..index..".")
		end
	end

	self:NextThink(CurTime())
	return true
end

function ENT:Draw()
	local index = self:GetInternalIndex()

	local m = Matrix()
	m:Translate(self:GetPos())

	cam.PushModelMatrix(m)
	IMPORTS.voxDraw(index)
	cam.PopModelMatrix()
end

function ENT:TestCollision(start,delta,isbox,extents)
	if isbox then
		--debugoverlay.Box(start,Vector(-extents.x,-extents.y,0),Vector(extents.x,extents.y,extents.z*2),.05,Color(255,255,0,0))
	end
	start=self:WorldToLocal(start)
	local index = self:GetInternalIndex()
	local fraction,hitpos,normal = IMPORTS.voxTrace(index,start,delta,isbox,extents)
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
		IMPORTS.voxGenerate(index,f)
		IMPORTS.voxReInit(index)
		IMPORTS.voxFlagAllChunksForUpdate(index)
	end

	function ENT:save(file_name)
		local index = self:GetInternalIndex()

		local dims = CONFIGS[index].dimensions or Vector(1,1,1)

		local file_handle = file.Open(file_name,"wb","DATA")
		if !file_handle then return false end

		file_handle:Write("VolFile0")
		file_handle:WriteShort(dims.x)
		file_handle:WriteShort(dims.y)
		file_handle:WriteShort(dims.z)

		local i=0
		while true do
			local data = IMPORTS.voxData(index,i)
			if isbool(data) then
				if data then break end
				file_handle:Close()
				file.Delete(file_name)
				return false
			end
			file_handle:WriteShort(#data)
			file_handle:Write(data)
			i=i+1
		end

		file_handle:Close()


		return true
	end

	function ENT:load(file_name)
		local index = self:GetInternalIndex()

		local dims = CONFIGS[index].dimensions or Vector(1,1,1)

		local file_handle = file.Open(file_name,"rb","DATA")
		if !file_handle then return false end

		if file_handle:Read(8)~="VolFile0" then file_handle:Close() return false end
		local fdx = file_handle:ReadShort()
		local fdy = file_handle:ReadShort()
		local fdz = file_handle:ReadShort()

		if fdx~=dims.x or fdy~=dims.y or fdz~=dims.z then file_handle:Close() return false end

		for i=0,fdx*fdy*fdz-1 do
			local len = file_handle:ReadShort() --TODO lots of stuff here could still break
			local data = file_handle:Read(len)
			IMPORTS.voxInitChunk(index,i,data)
		end

		file_handle:Close()

		IMPORTS.voxReInit(index)
		IMPORTS.voxFlagAllChunksForUpdate(index)

		return true
	end

	function ENT:getBlock(x,y,z)
		local index = self:GetInternalIndex()
		return IMPORTS.voxGet(index,x,y,z)
	end

	function ENT:setBlock(x,y,z,d)
		local index = self:GetInternalIndex()
		local success = IMPORTS.voxSet(index,x,y,z,d)
		if success then
			net.Start("voxelate_set")
			net.WriteUInt(index,16)
			net.WriteUInt(x,16)
			net.WriteUInt(y,16)
			net.WriteUInt(z,16)
			net.WriteUInt(d,16)
			net.Broadcast()
		end
	end

	function ENT:getAt(pos)
		local index = self:GetInternalIndex()
		local scale = CONFIGS[index].scale or 1

		local rel_pos = self:WorldToLocal(pos)/scale
		return self:getBlock(rel_pos.x,rel_pos.y,rel_pos.z)
	end

	function ENT:setAt(pos,d)
		local index = self:GetInternalIndex()
		local scale = CONFIGS[index].scale or 1

		local rel_pos = self:WorldToLocal(pos)/scale
		self:setBlock(rel_pos.x,rel_pos.y,rel_pos.z,d)
	end

	function ENT:setRegion(x,y,z,sx,sy,sz,d) --allow d to be string data
		local index = self:GetInternalIndex()
		local set = IMPORTS.voxSet

		local fix = math.floor

		x = fix(x)
		y = fix(y)
		z = fix(z)
		sx = fix(sx)
		sy = fix(sy)
		sz = fix(sz)

		for ix=x,x+sx do
			for iy=y,y+sy do
				for iz=z,z+sz do
					set(index,ix,iy,iz,d)
				end
			end
		end

		net.Start("voxelate_set_region")
		net.WriteUInt(index,16)
		net.WriteInt(x,16)
		net.WriteInt(y,16)
		net.WriteInt(z,16)
		net.WriteUInt(sx,16)
		net.WriteUInt(sy,16)
		net.WriteUInt(sz,16)
		net.WriteUInt(d,16)
		net.Broadcast()
	end

	function ENT:setRegionAt(v1,v2,d)
		local index = self:GetInternalIndex()
		local scale = CONFIGS[index].scale or 1

		local lower=self:WorldToLocal(v1)/scale
		local upper=self:WorldToLocal(v2)/scale

		OrderVectors(lower,upper)

		local fix = math.floor

		self:setRegion(lower.x,lower.y,lower.z,fix(upper.x)-fix(lower.x),fix(upper.y)-fix(lower.y),fix(upper.z)-fix(lower.z),d)
	end

	function ENT:setSphere(x,y,z,r,d)
		local index = self:GetInternalIndex()
		local set = IMPORTS.voxSet

		local fix = math.floor

		x = fix(x)
		y = fix(y)
		z = fix(z)
		r = fix(r)

		local rsqr = r*r
		for ix=x-r,x+r do
			local xsqr = (ix-x)*(ix-x)
			for iy=y-r,y+r do
				local xysqr = xsqr+(iy-y)*(iy-y)
				for iz=z-r,z+r do
					local xyzsqr = xysqr+(iz-z)*(iz-z)
					if xyzsqr<=rsqr then
						set(index,ix,iy,iz,d)
					end
				end
			end
		end

		net.Start("voxelate_set_sphere")
		net.WriteUInt(index,16)
		net.WriteUInt(x,16)
		net.WriteUInt(y,16)
		net.WriteUInt(z,16)
		net.WriteUInt(r,16)
		net.WriteUInt(d,16)
		net.Broadcast()
	end

	function ENT:setSphereAt(pos,r,d)
		local index = self:GetInternalIndex()
		local scale = CONFIGS[index].scale or 1

		pos=self:WorldToLocal(pos)/scale
		r=r/scale

		self:setSphere(pos.x,pos.y,pos.z,r,d)
	end
end

function ENT:UpdateTransmitState()
	return TRANSMIT_ALWAYS
end

hook.Add("Initialize","voxelate_init",function()
	scripted_ents.Register(ENT,"voxels")
	if SERVER then hook.Call("VoxelateReady") end
end)

hook.Add("PhysgunPickup", "voxelate_nograb", function(ply,ent)
	if ent:GetClass() == "voxels" then return false end
end)

hook.Add("CanTool", "voxelate_notool", function(ply,tr,tool)
	if IsValid( tr.Entity ) and tr.Entity:GetClass() == "voxels" then return false end
end)
