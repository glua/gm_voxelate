local runtime,exports = ...

local NetworkChannel = runtime.oop.create("NetworkChannel")
exports.NetworkChannel = NetworkChannel

local NetworkPacket = runtime.require("./packet").NetworkPacket

function NetworkChannel:__ctor(channelName,channelID)
    self.channelName = channelName
    self.channelID = channelID
end

function NetworkChannel:BindToRouter(router)
    self.router = router

    self.router:RegisterChannelID(self.channelName,self.channelID)

    return self
end

function NetworkChannel:NewPacket(unreliable)
    return NetworkPacket:__new(false,self,unreliable)
end

function NetworkChannel:OnDataInternal(data,peerID)
    local packet = NetworkPacket:__new(true,self,false,peerID,data)

    self:OnIncomingPacket(packet)
end

function NetworkChannel:OnIncomingPacket()
    error("Unimplemented: NetworkChannel:OnIncomingPacket()")
end
