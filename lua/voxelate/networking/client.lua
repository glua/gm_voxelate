local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

local SharedRouter = runtime.require("./shared").Router
runtime.oop.extend(Router,SharedRouter)

function Router:__ctor(voxelate)
    self.__parent.__ctor(self,voxelate)

    hook.Add("InitPostEntity","Voxelate.Networking.BeginSyncSequence",function()
        net.Start("gmod_vox_sync")
        net.SendToServer()
    end)

    net.Receive("gmod_vox_sync",function(len)
        local PUID = net.ReadString()

        self.clientPUID = PUID

        self:StartENet()
    end)
end

function Router:StartENet()
    self.voxelate.io:PrintDebug("Starting ENet with PUID [%s]...",self.clientPUID)
end
