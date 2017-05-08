local runtime = dofile("runtime.lua")

print("HELLO WORLD!")

print("sn_bf_write",sn_bf_write)

gm_voxelate = runtime.require("./voxelate").Voxelate

gm_voxelate.io:SetDebug(true)

runtime.require("./voxelate/entities/voxelworld")
