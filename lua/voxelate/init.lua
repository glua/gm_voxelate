
require("bulkhelpers")

require("networking")
require("entities/voxelworld")

-- Don't let players screw with our voxels! 
hook.Add("PhysgunPickup", "Voxelate.NoPhysgun", function(ply,ent) 
  if ent:GetClass() == "voxel_world" then return false end 
end) 
 
hook.Add("CanTool", "Voxelate.NoTool", function(ply,tr,tool) 
  if IsValid( tr.Entity ) and tr.Entity:GetClass() == "voxel_world" then return false end 
end)
