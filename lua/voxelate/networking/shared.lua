local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

function Router:__ctor(voxelate)
    self.voxelate = voxelate

    self.channels = {}
    self.channelsEx = {}

    self.channelListeners = {}

    hook.Add("Tick","Voxelate.Networking.Polling",function()
        self.voxelate.module.networkPoll()
    end)

    self:RegisterChannelID("AuthHandshake",1)
end

function Router:GeneratePUID()
    -- PUID = Pseudo-unique identifier

    return string.format(
        "%08x-%04x-%04x-%08x",
        math.random(2 ^ 32),
        math.random(2 ^ 16),
        math.random(2 ^ 16),
        math.random(2 ^ 32)
    )
end

function Router:RegisterChannelID(channelName,channelID)
    assert(self.channels[channelID],"Channel ID "..channelID.." is already being used by "..self.channels[channelID])

    self.channels[channelID] = channelName
    self.channelsEx[channelName] = channelID
end

function Router:Listen(channelName,callback)
    assert(self.channelsEx[channelName],"Unknown channel: "..channelName)

    self.channelListeners[self.channelsEx[channelName]] = callback
end

function Router:PropagateMessage(peerID,channelID,payloadData)
    if self.channelListeners[channelID] then
        self.channelListeners[channelID](payloadData,peerID)
    else
        self.voxelate.io:PrintDebug("Unhandled message in channel ID %d",channelID)
    end
end
