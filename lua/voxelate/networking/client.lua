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

    local serverIP = game.GetIPAddress():gsub(":%d+$","")
    if serverIP == "loopback" then serverIP = "127.0.0.1" end

    self.voxelate.io:PrintDebug("Connecting to %s...",serverIP)

    local suc,err = self.voxelate.module.networkConnect(serverIP)

    if suc then
        self.voxelate.io:Print("Now connected to %s via ENet!",serverIP)
    else
        self.voxelate.io:PrintError("Couldn't connect to %s via ENet: %s",serverIP,tostring(err))
    end
end
