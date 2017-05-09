local runtime = dofile("runtime.lua")

gm_voxelate = runtime.require("./voxelate").Voxelate

gm_voxelate.io:SetDebug(true)

runtime.require("./voxelate/entities/voxelworld")

gm_voxelate.io:Print("Lua initialisation complete")
