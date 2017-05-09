local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

local SharedRouter = runtime.require("./shared").Router
runtime.oop.extend(Router,SharedRouter)

local bitbuf = runtime.require("../bitbuffer")

function Router:__ctor(voxelate)
    self.super:__ctor(voxelate)

    hook.Add("InitPostEntity","Voxelate.Networking.BeginSyncSequence",function()
        net.Start("gmod_vox_sync")
        net.SendToServer()
    end)

    hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function()
        self.voxelate.io:PrintDebug("We have disconnected from the server...")
    end)

    hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,data,channelID)
        self:IncomingPacket(data,channelID)
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
    if serverIP == "loopback" then serverIP = "localhost" end

    self.voxelate.io:PrintDebug("Connecting to %s...",serverIP)

    local suc,err = self.voxelate.module.networkConnect(serverIP)

    if suc then
        self.voxelate.io:Print("Now connected to %s via ENet!",serverIP)

        self:SendInChannel("AuthHandshake",self.clientPUID)
    else
        self.voxelate.io:PrintError("Couldn't connect to %s via ENet: %s",serverIP,tostring(err))
    end
end

function Router:IncomingPacket(data,internalChannelID)
    local reader = bitbuf.Reader(data)

    local channelID = reader:ReadUInt(16)
    local payload,ok = reader:ReadBytes(#data - 2)

    if not ok then
        error("Packet read failure")
    end

    self:PropagateMessage(false,channelID,payload:GetString())
end

function Router:SendInChannel(channelName,payloadData,unreliable)
    assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

    local buf = bitbuf.Writer(#payloadData + 2)
    local payload = UCHARPTR_FromString(payloadData,#payloadData)

    buf:WriteUInt(self.channelsEx[channelName],16)
    buf:WriteBytes(payload,#payloadData)

    local data = buf:GetWrittenString()

    self.voxelate.module.networkSendPacket(data,#data,unreliable)
end
