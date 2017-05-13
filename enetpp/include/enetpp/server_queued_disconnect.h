#ifndef ENETPP_SERVER_QUEUED_DISCONNECT_H_
#define ENETPP_SERVER_QUEUED_DISCONNECT_H_

#include "enet/enet.h"

namespace enetpp {

	class server_queued_disconnect{
	public:
		enet_uint8 _client_id;
		bool _force;

	public:
		server_queued_disconnect()
			: _client_id(0)
			, _force(false) {
		}

		server_queued_disconnect(enet_uint8 client_id, bool force)
			: _client_id(client_id)
			, _force(force) {
		}
	};

}

#endif