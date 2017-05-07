local IMPORTS = gm_voxelate.IMPORTS

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
    IMPORTS.voxEnableMeshGeneration(index,true)
    IMPORTS.voxFlagAllChunksForUpdate(index)
end)

net.Receive("voxelate_init_restart",function(len)
    local index = net.ReadUInt(16)
    IMPORTS.voxEnableMeshGeneration(index,false)
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
    local x = net.ReadInt(16)
    local y = net.ReadInt(16)
    local z = net.ReadInt(16)
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
