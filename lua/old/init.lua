local IMPORTS = G_VOX_IMPORTS
G_VOX_IMPORTS = nil

gm_voxelate = {}
gm_voxelate.IMPORTS = IMPORTS

gm_voxelate.CONFIGS = {}

gm_voxelate.VERSION = IMPORTS.VERSION

if CLIENT then
	CreateClientConVar("voxelate_version" , gm_voxelate.VERSION, false, true)
else
	local function parseVersion(ver)
		ver=string.Explode(".",ver or "",false)
		return tonumber(ver[1] or -1),tonumber(ver[2] or -1),tonumber(ver[3] or -1)
	end

	local MASTER_MAJOR, MASTER_MINOR, MASTER_PATCH = parseVersion(gm_voxelate.VERSION)

	hook.Add("PlayerInitialSpawn","voxelate_version_check",function(ply)

		local CLIENT_STR = ply:GetInfo("voxelate_version")

		if (CLIENT_STR=="") then
			ply:Kick("Voxelate not installed.\n\nSERVER VERSION: "..gm_voxelate.VERSION.."\n\nSorry")
		end

		local CLIENT_MAJOR, CLIENT_MINOR, CLIENT_PATCH = parseVersion(CLIENT_STR)

		if (CLIENT_MAJOR ~= MASTER_MAJOR or CLIENT_MINOR<MASTER_MINOR) then
			ply:Kick("Voxelate version mismatch.\n\nSERVER VERSION: "..gm_voxelate.VERSION.."\nYOUR VERSION: "..CLIENT_STR.."\n\nSorry")
		end
	end)
end


if SERVER then
	dofile("sv_networking.lua")
else
	dofile("cl_networking.lua")
end

dofile("entities/voxels.lua")
