local runtime,exports = ...

local DEFAULT_NETWORK_PACKET_SIZE = 25 * 1024 -- 25 KB

local NetworkPacket = runtime.oop.create("NetworkPacket")
exports.NetworkPacket = NetworkPacket

local bitbuf = runtime.require("../bitbuffer")

local StaticPacketBufferMeta = {}
local _R = debug.getregistry()
debug.setmetatable(StaticPacketBufferMeta,{__index = _R.sn_bf_write})

function StaticPacketBufferMeta:Send()
    return self.packet:Send(self)
end

function StaticPacketBufferMeta:Broadcast()
    return self.packet:Broadcast(self)
end

for key,val in pairs(_R.sn_bf_write) do
    if key:sub(1,2) == "__" then
        StaticPacketBufferMeta[key] = val
    end
end

StaticPacketBufferMeta.__index = StaticPacketBufferMeta

function NetworkPacket:__ctor(incoming,channel,unreliable,peerID,data)
    self.incoming = incoming
    self.channel = channel
    self.unreliable = unreliable
    self.peerID = peerID
    self.data = data
end

function NetworkPacket:GetPeer()
    return self.peerID
end

function NetworkPacket:SetPeer(peerID)
    assert(not self.incoming,"Peers cannot be set in incoming packets")
    assert(SERVER,"Packets can be targeted in serverside only")

    self.isMultiPeer = type(peerID) == "table"

    self.peerID = peerID

    return self
end

function NetworkPacket:GetBuffer(size)
    if self.incoming then
        return bitbuf.Reader(self.data)
    else
        local buffer = bitbuf.Writer(size or DEFAULT_NETWORK_PACKET_SIZE)

        debug.setmetatable(buffer,StaticPacketBufferMeta)

        buffer.packet = self

        return buffer
    end
end

function NetworkPacket:SendBuffer(buffer)
    assert(not self.incoming,"Incoming packets cannot be sent")
    local data = buffer:GetWrittenString()

    if SERVER then
        assert(self.peerID,"attempt to send packet to unknown destination")
        if self.isMultiPeer then
            for _,peerID in ipairs(self.peerID) do
                self.channel.router:SendInChannel(self.channel.channelName,data,peerID,self.unreliable)
            end
        else
            self.channel.router:SendInChannel(self.channel.channelName,data,self.peerID,self.unreliable)
        end
    else
        self.channel.router:SendInChannel(self.channel.channelName,data,self.unreliable)
    end

    return self
end

function NetworkPacket:BroadcastBuffer(buffer)
    assert(not self.incoming,"Incoming packets cannot be sent")
    assert(SERVER,"Packets can be broadcasted in serverside only")

    local data = buffer:GetWrittenString()

    self.channel.router:BroadcastInChannel(self.channel.channelName,data,self.peerID,self.unreliable)

    return self
end
