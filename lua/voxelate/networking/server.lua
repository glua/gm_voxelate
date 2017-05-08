local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

local SharedRouter = runtime.require("./shared")
runtime.oop.extend(Router,SharedRouter)

function Router:__ctor(voxelate)
    self.__parent.__ctor(self,voxelate)

    util.AddNetworkString("gmod_vox_sync")

    net.Receive("gmod_vox_sync",function(len,ply)
        self.voxelate.io:PrintDebug("Network synchronisation sequence starting for %s [%s]",ply:Nick(),ply:SteamID())

        local allocatedPUID = self:GeneratePUID()

        self.voxelate.io:PrintDebug("Allocated PUID [%s] for %s [%s]",allocatedPUID,ply:Nick(),ply:SteamID())

        net.Start("gmod_vox_sync")
            net.WriteString(allocatedPUID)
        net.Send(ply)
    end)
end
