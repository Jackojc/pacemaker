#ifndef BEATGEN_LIP_HPP
#define BEATGEN_LIP_HPP

#include <memory>

extern "C" {
	#include <jack/jack.h>
	#include <jack/midiport.h>
	#include <jack/ringbuffer.h>
}

namespace bg {

	struct jack_deleter {
		template <typename T> constexpr void operator()(T arg) const {
			jack_free(arg);
		}
	};

	using JackPorts = std::unique_ptr<const char*[], jack_deleter>;

	struct JackData {
		jack_client_t* client = nullptr;
		jack_port_t* port = nullptr;

		jack_nframes_t sample_rate = 0;
		jack_nframes_t buffer_size = 0;
		uint64_t time = 0u;

		// cane::Timeline::iterator it;
		// cane::Timeline::const_iterator end;

		// cane::Timeline events;

		~JackData() {
			if (client != nullptr)
				jack_deactivate(client);

			if (port != nullptr)
				jack_port_unregister(client, port);

			if (client != nullptr)
				jack_client_close(client);
		}
	};

}

#endif

