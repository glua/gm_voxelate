#pragma once

const char* LUA_SRC = R">>(

local IMPORTS = G_VOX_IMPORTS
G_VOX_IMPORTS = nil

local CONFIGS = {}

local VERSION = IMPORTS.VERSION

if CLIENT then
	CreateClientConVar("voxelate_version" , VERSION, false, true)
else
	local function parseVersion(ver)
		ver=string.Explode(".",ver or "",false)
		return tonumber(ver[1] or -1),tonumber(ver[2] or -1),tonumber(ver[3] or -1)
	end

	local MASTER_MAJOR, MASTER_MINOR, MASTER_PATCH = parseVersion(VERSION)

	hook.Add("PlayerInitialSpawn","voxelate_version_check",function(ply)
		local CLIENT_STR = ply:GetInfo("voxelate_version")
		
		if (CLIENT_STR=="") then
			ply:Kick("Voxelate not installed.\n\nSERVER VERSION: "..VERSION.."\n\nSorry")
		end

		local CLIENT_MAJOR, CLIENT_MINOR, CLIENT_PATCH = parseVersion(CLIENT_STR)
		
		if (CLIENT_MAJOR != MASTER_MAJOR || CLIENT_MINOR<MASTER_MINOR) then
			ply:Kick("Voxelate version mismatch.\n\nSERVER VERSION: "..VERSION.."\nYOUR VERSION: "..CLIENT_STR.."\n\nSorry")
		end
	end)
end










if SERVER then
	util.AddNetworkString("voxelate_req")

	util.AddNetworkString("voxelate_init_start")
	util.AddNetworkString("voxelate_init_chunks")
	util.AddNetworkString("voxelate_init_finish")

	util.AddNetworkString("voxelate_explicit_delete")

	util.AddNetworkString("voxelate_set")
	util.AddNetworkString("voxelate_set_region")
	util.AddNetworkString("voxelate_set_sphere")

	local INIT_TASKS = {}

	hook.Add("Tick","voxelate_stream_tick",function()
		local to_delete = {}
		for ply,tasks in pairs(INIT_TASKS) do
			if !IsValid(ply) then table.insert(to_delete,ply) continue end
			local to_delete = {}
			for index,step in pairs(tasks) do
				local size_limit = 2000
				local size_written = 0
				net.Start("voxelate_init_chunks")
				net.WriteUInt(index,16)
				while size_written<size_limit do
					local data = IMPORTS.voxData(index,step)				
					if !data then table.insert(to_delete,index) break end

					net.WriteUInt(step,16)
					net.WriteUInt(#data,16)
					net.WriteData(data,#data)

					size_written = size_written + #data
					step = step+1
				end
				net.WriteUInt(65535,16)
				net.Send(ply)
				tasks[index] = step
			end
			for _,v in pairs(to_delete) do
				tasks[v]=nil
				net.Start("voxelate_init_finish")

				net.WriteUInt(v,16)
				net.Send(ply)
			end
		end
		for _,v in pairs(to_delete) do
			INIT_TASKS[v]=nil
		end
	end)

	net.Receive("voxelate_req",function(len,ply)
		local index = net.ReadUInt(16)

		if CONFIGS[index] then
			
			net.Start("voxelate_init_start")
			net.WriteUInt(index,16)
			net.WriteTable(CONFIGS[index])
			net.Send(ply)

			if !INIT_TASKS[ply] then
				INIT_TASKS[ply]={}
			end

			INIT_TASKS[ply][index]=0
		end
	end)
else
	net.Receive("voxelate_init_start",function(len)
		local index = net.ReadUInt(16)
		local config = net.ReadTable()

		IMPORTS.voxInit(index,config)
	end)

	net.Receive("voxelate_init_chunks",function(len)
		local index = net.ReadUInt(16)
		while true do		
			local chunk_n = net.ReadUInt(16)
			if chunk_n==65535 then break end

			local data_len = net.ReadUInt(16)
			local data = net.ReadData(data_len)
			IMPORTS.voxInitChunk(index,chunk_n,data)
		end
	end)

	net.Receive("voxelate_init_finish",function(len)
		local index = net.ReadUInt(16)
		IMPORTS.voxInitFinish(index)
	end)

	net.Receive("voxelate_explicit_delete",function(len)
		local index = net.ReadUInt(16)
		IMPORTS.voxDelete(index)
	end)

	net.Receive("voxelate_set",function(len)
		local index = net.ReadUInt(16)
		local x = net.ReadUInt(16)
		local y = net.ReadUInt(16)
		local z = net.ReadUInt(16)
		local d = net.ReadUInt(16)

		IMPORTS.voxSet(index, x, y, z, d)
	end)

	net.Receive("voxelate_set_region",function(len)
		local index = net.ReadUInt(16)
		local x = net.ReadUInt(16)
		local y = net.ReadUInt(16)
		local z = net.ReadUInt(16)
		local sx = net.ReadUInt(16)
		local sy = net.ReadUInt(16)
		local sz = net.ReadUInt(16)
		local d = net.ReadUInt(16)

		local set = IMPORTS.voxSet
		for ix=x,x+sx do
			for iy=y,y+sy do
				for iz=z,z+sz do
					set(index,ix,iy,iz,d)
				end
			end
		end
	end)

	net.Receive("voxelate_set_sphere",function(len)
		local index = net.ReadUInt(16)
		local x = net.ReadUInt(16)
		local y = net.ReadUInt(16)
		local z = net.ReadUInt(16)
		local r = net.ReadUInt(16)
		local d = net.ReadUInt(16)		
	
		local set = IMPORTS.voxSet
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
	end)
end





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

	self:SetCollisionBounds(mins,maxs)
	if CLIENT then
		self:SetRenderBounds(mins,maxs)
	end
end

function ENT:Think()
	local index = self:GetInternalIndex()	
	IMPORTS.voxUpdate(index,10,self,self:GetPos())

	self:NextThink(CurTime())
	return true
end

local mat = Material("models/wireframe")

function ENT:Draw()
	local index = self:GetInternalIndex()

	local m = Matrix()
	m:Translate(self:GetPos())

	cam.PushModelMatrix(m)
	render.SetMaterial(mat)
	local lower, upper = self:GetRenderBounds()
	render.DrawBox(Vector(),Angle(), upper, lower)
	render.DrawBox(Vector(),Angle(), Vector(10,10,10), Vector(-10,-10,-10))
	IMPORTS.voxDraw(index)
	cam.PopModelMatrix()
end

ENT.TestCollision = IMPORTS.ENT_TestCollision

if SERVER then
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
		net.WriteUInt(x,16)
		net.WriteUInt(y,16)
		net.WriteUInt(z,16)
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

scripted_ents.Register(ENT,"voxels")




hook.Add("PhysgunPickup", "voxelate_nograb", function(ply,ent)
	if ent:GetClass() == "voxels" then return false end
end)

hook.Add("CanTool", "voxelate_notool", function(ply,tr,tool)
	if IsValid( tr.Entity ) and tr.Entity:GetClass() == "voxels" then return false end
end)

)>>";