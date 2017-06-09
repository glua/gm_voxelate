local runtime = dofile("runtime.lua")

gm_voxelate = runtime.require("./voxelate")

gm_voxelate.io:SetDebug(true)

print("Lua initialization complete.")

-- Why?
--[[function gm_voxelate:GetVoxelateRuntime()
	return runtime
end

function gm_voxelate:GetNewRuntime()
	return dofile("runtime.lua")
end]]
