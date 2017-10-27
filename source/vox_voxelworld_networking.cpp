#include "vox_voxelworld.h"
#include "vox_chunk.h"
#include "vox_network.h"

struct SingleChunkMessage : BaseNetworkMessage {
	uint8_t channelID = vox_network_channel::chunk;
	uint8_t worldID;
	VoxelCoordXYZ pos;
	char chunkData[CHUNK_BUFFER_SIZE] { 0 };
};

void voxelworld_initialise_networking_static() {
#ifdef VOXELATE_CLIENT
	vox_networking::channelListen(vox_network_channel::chunk, [&](int peerID, const char* data, size_t data_len) {
		if (data_len < (sizeof(SingleChunkMessage) - CHUNK_BUFFER_SIZE)) {
			// message too short
			// bad!!!!
			return;
		}

		if (data_len > sizeof(SingleChunkMessage)) {
			// message too long
			// bad!!!!
			return;
		}

		SingleChunkMessage msg;
		memcpy(&msg, data, data_len);

		auto world = getIndexedVoxelWorld(msg.worldID);

		if (world == NULL) {
			return;
		}

		world->setChunkData(
			msg.pos[0],
			msg.pos[1],
			msg.pos[2],
			msg.chunkData,
			CHUNK_BUFFER_SIZE - sizeof(SingleChunkMessage) + data_len
		);
	});
#endif
}

#ifdef VOXELATE_SERVER
bool VoxelWorld::sendChunk(int peerID, VoxelCoordXYZ pos) {
	SingleChunkMessage msg;

	msg.worldID = worldID;
	msg.pos = pos;

	int compressed_size = getChunkData(pos[0], pos[1], pos[2], msg.chunkData);

	if (compressed_size == 0)
		return false;

	return vox_networking::channelSend(
		peerID,
		&msg,
		sizeof(SingleChunkMessage) - CHUNK_BUFFER_SIZE + compressed_size
	);
}
#endif
