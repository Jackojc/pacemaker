#ifndef PACEMAKER_HPP
#define PACEMAKER_HPP

#include <memory>

extern "C" {
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
}

#include <pacemaker/util.hpp>

namespace pacemaker {
	int sample_rate_callback(jack_nframes_t new_sample_rate, void* arg);
	int buffer_size_callback(jack_nframes_t new_buffer_size, void* arg);

	namespace detail {
		struct JackClientDeleter {
			constexpr void operator()(jack_client_t* client) const noexcept {
				jack_free(client);
			}
		};

		struct JackPortDeleter {
			jack_client_t* client = nullptr;

			template <typename T>
			constexpr void operator()(jack_port_t* port) const noexcept {
				jack_port_unregister(client, port);
			}
		};

		using JackClient =
			std::unique_ptr<jack_client_t, detail::JackClientDeleter>;

		using JackPort = std::unique_ptr<jack_port_t, detail::JackPortDeleter>;
	}  // namespace detail

	struct JackConnection {
		jack_client_t* client = nullptr;

		jack_nframes_t sample_rate = 0;
		jack_nframes_t buffer_size = 0;

		JackConnection() {
			if (not(client = PACEMAKER_DBG(jack_client_open(
						"pacemaker",
						JackOptions::JackNoStartServer,
						nullptr)))) {
				throw Fatal {};
			}

			if (PACEMAKER_DBG(jack_set_sample_rate_callback(
					client, sample_rate_callback, static_cast<void*>(this)))) {
				throw Fatal {};
			}

			if (PACEMAKER_DBG(jack_set_buffer_size_callback(
					client, buffer_size_callback, static_cast<void*>(this)))) {
				throw Fatal {};
			}

			buffer_size = PACEMAKER_DBG(jack_get_buffer_size(client));
			sample_rate = PACEMAKER_DBG(jack_get_sample_rate(client));
		}

		~JackConnection() {
			if (client != nullptr) {
				PACEMAKER_DBG(jack_deactivate(client));
			}

			if (client != nullptr) {
				PACEMAKER_DBG(jack_client_close(client));
			}
		}

		JackConnection(const JackConnection& conn) {}
	};

	int sample_rate_callback(jack_nframes_t new_sample_rate, void* arg) {
		auto& client = *static_cast<JackConnection*>(arg);
		client.sample_rate = new_sample_rate;
		return 0;
	}

	int buffer_size_callback(jack_nframes_t new_buffer_size, void* arg) {
		auto& client = *static_cast<JackConnection*>(arg);
		client.buffer_size = new_buffer_size;
		return 0;
	}

}  // namespace pacemaker

#endif
