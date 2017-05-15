local runtime = dofile("runtime.lua")

gm_voxelate = runtime.require("./voxelate").Voxelate

gm_voxelate.io:SetDebug(true)

gm_voxelate.io:Print("Lua initialisation complete.")

-- Why?
function gm_voxelate:GetVoxelateRuntime()
	return runtime
end

function gm_voxelate:GetNewRuntime()
	return dofile("runtime.lua")
end
