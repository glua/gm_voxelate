local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

local SharedRouter = runtime.require("./shared").Router
runtime.oop.extend(Router,SharedRouter)

function Router:__ctor(voxelate)
    self.__parent.__ctor(self,voxelate)

    self.PUIDs = setmetatable({},{__mode = "v"}) -- {[PUID] = Player}
    self.PUIDsEx = setmetatable({},{__mode = "k"}) -- {[Player] = PUID}
    self.PeerIDs = setmetatable({},{__mode = "v"}) -- {[PeerID] = Player}
    self.PeerIDsEx = setmetatable({},{__mode = "k"}) -- {[Player] = PUID}

    self.PeerAddress = {} -- {[PeerID] = IPAddress}

    hook.Add("VoxNetworkConnect","Voxelate.Networking",function(peerID,addressNum)
        local address = string.format(
            "%d.%d.%d.%d",
            bit.band(addressNum,0x000000FF),
            bit.rshift(bit.band(addressNum,0x0000FF00),8),
            bit.rshift(bit.band(addressNum,0x00FF0000),16),
            bit.rshift(bit.band(addressNum,0xFF000000),24)
        )

        self.voxelate.io:PrintDebug("A new ENet Peer [%d] has connected from %s",peerID,address)
        self.PeerAddress[peerID] = address
    end)

    hook.Add("VoxNetworkDisconnect","Voxelate.Networking",function(peerID)
        self.PeerAddress[peerID] = nil
        local ply = self.PeerIDs[peerID]
        self.PeerIDs[peerID] = nil
        if ply then
            self.PeerIDsEx[ply] = nil
            self.voxelate.io:PrintDebug("An ENet Peer [%d] has disconnected (%s [%s])",peerID,ply:Nick(),ply:SteamID())
        else
            self.voxelate.io:PrintDebug("An unauthenticated ENet Peer [%d] has disconnected",peerID)
        end
    end)

    hook.Add("VoxNetworkPacket","Voxelate.Networking",function(peerID,data,channelID)
        self:IncomingPacket(peerID,data,channelID)
    end)

    util.AddNetworkString("gmod_vox_sync")

    net.Receive("gmod_vox_sync",function(len,ply)
        self.voxelate.io:PrintDebug("Network synchronisation sequence starting for %s [%s]",ply:Nick(),ply:SteamID())

        local allocatedPUID
        repeat
            allocatedPUID = self:GeneratePUID()
        until not self.PUIDs[allocatedPUID]

        self.PUIDs[allocatedPUID] = ply
        self.PUIDsEx[ply] = allocatedPUID

        self.voxelate.io:PrintDebug("Allocated PUID [%s] for %s [%s]",allocatedPUID,ply:Nick(),ply:SteamID())

        net.Start("gmod_vox_sync")
            net.WriteString(allocatedPUID)
        net.Send(ply)
    end)
end
