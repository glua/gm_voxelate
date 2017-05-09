local runtime,exports = ...

local serialization = runtime.require("../serialization")

local NetworkChannel = runtime.require("../networking/channel").NetworkChannel

local VoxelWorldChunkStreamChannel = runtime.oop.create("VoxelWorldChunkStreamChannel")
exports.VoxelWorldChunkStreamChannel = VoxelWorldChunkStreamChannel
runtime.oop.extend(VoxelWorldChunkStreamChannel,NetworkChannel)

local P = {
    CHUNK_FULL_UPDATE = 1,
    CHUNK_BLOCK_UPDATE = 2,
}

function VoxelWorldChunkStreamChannel:SendFullChunk(peerID,worldID,chunkX,chunkY,chunkZ)
    assert(SERVER,"serverside only")

    self.voxelate.io:PrintDebug("Sending voxel config for %d to %d...",worldID,peerID)

    local packet = self:NewPacket()
    packet:SetPeer(peerID)

    local buffer = packet:GetBuffer()

    local data = self.voxelate.module.voxData(worldID,chunkX,chunkY,chunkZ)
    if data == true then return end

    buffer:WriteInt(worldID,32)
    buffer:WriteUInt(P.CHUNK_FULL_UPDATE,8)
    buffer:WriteInt(chunkX,32)
    buffer:WriteInt(chunkY,32)
    buffer:WriteInt(chunkZ,32)

    buffer:WriteString(serialization.serialize(config))

    return buffer:Send()
end

function VoxelWorldChunkStreamChannel:ReceiveFullChunk(worldID,buffer)
    assert(CLIENT,"clientside only")

    self.voxelate.io:PrintDebug("Receiving voxel config for %d...",worldID)

    local config = serialization.deserialize(buffer:ReadString())

    self.voxelate:SetWorldConfig(worldID,config)
end

function VoxelWorldChunkStreamChannel:OnIncomingPacket(packet)
    local buffer = packet:GetBuffer()

    local worldID = buffer:ReadUInt(8)
    local requestID = buffer:ReadUInt(8)

    if SERVER then
        if requestID == P.CHUNK_FULL_UPDATE then
            -- time to send world config
            self:SendFullChunk(packet:GetPeer(),worldID)
        end
    else
        if requestID == P.CHUNK_FULL_UPDATE then
            self:ReceiveFullChunk(worldID,buffer)
        end
    end
end
