local runtime,exports = ...

local serialization = runtime.require("../serialization")

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local VoxelWorldInitChannel = runtime.oop.create("VoxelWorldInitChannel")
exports.VoxelWorldInitChannel = VoxelWorldInitChannel
runtime.oop.extend(VoxelWorldInitChannel,NetworkChannel)

local P = {
    VOXELATE_WORLD_CONFIG = 1,
}

function VoxelWorldInitChannel:RequestVoxelWorldConfig(worldID)
    assert(CLIENT,"clientside only")

    self.voxelate.io:PrintDebug("Requesting voxel config for %d...",worldID)

    local packet = self:NewPacket()

    local buffer = packet:GetBuffer()

    buffer:WriteUInt(worldID,8)
    buffer:WriteUInt(P.VOXELATE_WORLD_CONFIG,8)

    return buffer:Send()
end

function VoxelWorldInitChannel:SendVoxelWorldConfig(peerID,worldID)
    assert(SERVER,"serverside only")

    self.voxelate.io:PrintDebug("Sending voxel config for %d to %d...",worldID,peerID)

    local packet = self:NewPacket()
    packet:SetPeer(peerID)

    local buffer = packet:GetBuffer()

    local config = self.voxelate:GetWorldConfig(worldID) or {}

    buffer:WriteUInt(worldID,8)
    buffer:WriteUInt(P.VOXELATE_WORLD_CONFIG,8)

    buffer:WriteString(serialization.serialize(config))

    return buffer:Send()
end

function VoxelWorldInitChannel:ReceiveVoxelWorldConfig(worldID,buffer)
    assert(CLIENT,"clientside only")

    self.voxelate.io:PrintDebug("Receiving voxel config for %d...",worldID)

    local config = serialization.deserialize(buffer:ReadString())

    self.voxelate:SetWorldConfig(worldID,config)

	gm_voxelate.module.voxNewWorld(config,worldID)
end

function VoxelWorldInitChannel:OnIncomingPacket(packet)
    local buffer = packet:GetBuffer()

    local worldID = buffer:ReadUInt(8)
    local requestID = buffer:ReadUInt(8)

    if SERVER then
        if requestID == P.VOXELATE_WORLD_CONFIG then
            -- time to send world config
            self:SendVoxelWorldConfig(packet:GetPeer(),worldID)
        end
    else
        if requestID == P.VOXELATE_WORLD_CONFIG then
            self:ReceiveVoxelWorldConfig(worldID,buffer)
        end
    end
end
