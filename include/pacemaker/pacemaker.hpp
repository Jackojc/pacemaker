#ifndef PACEMAKER_HPP
#define PACEMAKER_HPP

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

extern "C" {
#include <jack/jack.h>
#include <jack/metadata.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/statistics.h>
#include <jack/types.h>
}

#include <pacemaker/const.hpp>
#include <pacemaker/util.hpp>

namespace pacemaker {
	struct JackClient;

	namespace detail {
		int sample_rate_callback(jack_nframes_t, void*);
		int buffer_size_callback(jack_nframes_t, void*);

		void client_registration_callback(const char*, int, void*);

		void port_connect_callback(jack_port_id_t, jack_port_id_t, int, void*);
		void port_registration_callback(jack_port_id_t, int, void*);
		void port_rename_callback(jack_port_id_t, const char*, const char*, void*);

		int xrun_callback(void*);

		// Cast void* argument to JackClient.
		JackClient& to_conn(void* arg) {
			return *static_cast<JackClient*>(arg);
		}
	}  // namespace detail

	struct JackPort {
		jack_client_t* client;
		jack_port_t* port;

		operator jack_port_t*() const {
			return port;
		}

		jack_port_t* get() const {
			return port;
		}

		JackPort(jack_client_t* client_, jack_port_t* port_): client(client_), port(port_) {}

		~JackPort() {
			PACEMAKER_DBG(jack_port_unregister(client, port));
		}

		JackPort(const JackPort& other): JackPort(other.client, other.port) {}

		JackPort(JackPort&& other) noexcept:
			client(std::exchange(other.client, nullptr)),
			port(std::exchange(other.port, nullptr)) {}

		JackPort& operator=(const JackPort& other) {
			return *this = JackPort(other);
		}

		JackPort& operator=(JackPort&& other) noexcept {
			std::swap(client, other.client);
			std::swap(port, other.port);
			return *this;
		}

		bool connect(const std::string& dst) {
			bool is_fail = PACEMAKER_DBG(jack_connect(client, jack_port_name(port), dst.c_str()));
			return not(is_fail);
		}

		bool disconnect(const std::string& dst) {
			bool is_fail = PACEMAKER_DBG(jack_disconnect(client, jack_port_name(port), dst.c_str()));
			return not(is_fail);
		}

		int connected() const {
			return jack_port_connected(port);
		}

		bool rename(const std::string& name) {
			bool is_fail = jack_port_rename(client, port, name.c_str());
			return not(is_fail);
		}

		void* get_buffer(jack_nframes_t frames) {
			return jack_port_get_buffer(port, frames);
		}

		// TODO: Function to get list of connections:
		// jack_port_get_connections()
	};

	struct JackClient {
		jack_client_t* client;

		jack_nframes_t sample_rate;
		jack_nframes_t buffer_size;

		std::function<int(jack_nframes_t, JackClient&)> callback;

		operator jack_client_t*() const {
			return client;
		}

		jack_client_t* get() const {
			return client;
		}

		JackClient(): client(nullptr), sample_rate(0), buffer_size(0) {
			jack_status_t flags;

			client = PACEMAKER_DBG(jack_client_open(
				STR_CLIENT_NAME, static_cast<JackOptions>(JackNoStartServer | JackUseExactName), &flags));

			PACEMAKER_DBG(flags);

			if (not client) {
				pacemaker::fatal_error("could not open client");
			}

			// constexpr auto check_flag = [&](jack_status_t mask, jack_status_t flag, std::string_view msg, auto&& fn)
			// { 	if (mask & flag) { 		fn(msg);
			// 	}
			// };

			// check_flag(flags, JackStatus::JackFailure, "general failure", pacemaker::fatal_error);
			// check_flag(flags, JackStatus::JackInvalidOption, "invalid or unsupported option",
			// pacemaker::fatal_error); check_flag(flags, JackStatus::JackNameNotUnique, "client name is not unique",
			// pacemaker::fatal_error); check_flag(flags, JackStatus::JackServerStarted, "jack server started",
			// pacemaker::warning); check_flag(flags, JackStatus::JackServerFailed, "unable to connect to jack server",
			// pacemaker::fatal_error); check_flag(flags, JackStatus::JackServerError, "communication error with jack
			// server", pacemaker::fatal_error); check_flag(flags, JackStatus::JackNoSuchClient, "requested client does
			// not exist", pacemaker::fatal_error); check_flag(flags, JackStatus::JackLoadFailure, "unable to load
			// internal client", pacemaker::fatal_error); check_flag(flags, JackStatus::JackShmFailure, "initialisation
			// failure", pacemaker::fatal_error); check_flag(flags, JackStatus::JackVersionError, "protocol version
			// mismatch", pacemaker::fatal_error); check_flag(flags, JackStatus::JackBackendError, "internal error",
			// pacemaker::fatal_error); check_flag(flags, JackStatus::JackClientZombie, "zombie client",
			// pacemaker::fatal_error);

			// Notify on changes to sample rate.
			// We need this information to be correct so we can properly keep
			// track of time.
			if (PACEMAKER_DBG(
					jack_set_sample_rate_callback(client, detail::sample_rate_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set sample rate callback");
			}

			// Notify on changes to buffer size.
			if (PACEMAKER_DBG(
					jack_set_buffer_size_callback(client, detail::buffer_size_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set buffer size callback");
			}

			// We don't necessarily need the following callbacks but they're
			// useful for logging purposes.

			// Notify when a new client is registered.
			if (PACEMAKER_DBG(jack_set_client_registration_callback(
					client, detail::client_registration_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set client registration callback");
			}

			// Notify when a port is connected.
			if (PACEMAKER_DBG(
					jack_set_port_connect_callback(client, detail::port_connect_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set port connect callback");
			}

			// Notify when a new port is registered.
			if (PACEMAKER_DBG(jack_set_port_registration_callback(
					client, detail::port_registration_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set port registration callback");
			}

			// Notify when a port is renamed.
			if (PACEMAKER_DBG(
					jack_set_port_rename_callback(client, detail::port_rename_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set port rename callback");
			}

			// Notify when an xrun occurs.
			if (PACEMAKER_DBG(jack_set_xrun_callback(client, detail::xrun_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set xrun callback");
			}

			buffer_size = PACEMAKER_DBG(jack_get_buffer_size(client));
			sample_rate = PACEMAKER_DBG(jack_get_sample_rate(client));

			// if (PACEMAKER_DBG(jack_activate(client))) {
			// 	pacemaker::fatal_error("could not activate client");
			// }
		}

		JackClient(jack_client_t* client_, jack_nframes_t sample_rate_, jack_nframes_t buffer_size_):
			client(client_),
			sample_rate(sample_rate_),
			buffer_size(buffer_size_) {}

		~JackClient() {
			PACEMAKER_DBG(jack_deactivate(client));
			PACEMAKER_DBG(jack_client_close(client));
		}

		JackClient(const JackClient& other): JackClient(other.client, other.sample_rate, other.buffer_size) {}

		JackClient(JackClient&& other) noexcept:
			client(std::exchange(other.client, nullptr)),
			sample_rate(std::exchange(other.sample_rate, 0)),
			buffer_size(std::exchange(other.buffer_size, 0)) {}

		JackClient& operator=(const JackClient& other) {
			return *this = JackClient(other);
		}

		JackClient& operator=(JackClient&& other) noexcept {
			std::swap(client, other.client);
			std::swap(sample_rate, other.sample_rate);
			std::swap(buffer_size, other.buffer_size);

			return *this;
		}

		JackPort port_register_output(const std::string& name) {
			return { client, jack_port_register(client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0) };
		}

		JackPort port_register_input(const std::string& name) {
			return { client, jack_port_register(client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0) };
		}

		bool is_mine(const JackPort& port) const {
			return jack_port_is_mine(client, port);
		}

		template <typename F> bool set_callback(F&& fn) {
			callback = fn;

			const auto wrapper = [](jack_nframes_t nframes, void* arg) {
				auto& client = detail::to_conn(arg);
				return client.callback(nframes, client);
			};

			bool is_fail = jack_set_process_callback(client, wrapper, this);
			return not(is_fail);
		}

		bool ready() {
			if (not callback) {
				pacemaker::fatal_error("no callback is set");
			}

			return jack_activate(client);
		}
	};

	// Callbacks
	namespace detail {
		int sample_rate_callback(jack_nframes_t new_sample_rate, void* arg) {
			PACEMAKER_LOG(LogLevel::WRN, "sample rate changed");
			detail::to_conn(arg).sample_rate = new_sample_rate;
			return 0;
		}

		int buffer_size_callback(jack_nframes_t new_buffer_size, void* arg) {
			PACEMAKER_LOG(LogLevel::WRN, "buffer size changed");
			detail::to_conn(arg).buffer_size = new_buffer_size;
			return 0;
		}

		void client_registration_callback(const char* client, int is_registering, void*) {
			constexpr std::array states { "unregistering", "registering" };
			PACEMAKER_LOG(LogLevel::WRN, client, " is ", states.at(is_registering));
		}

		void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int is_connecting, void* arg) {
			constexpr std::array states { "disconnecting from", "connecting to" };

			jack_port_t* port_a = jack_port_by_id(detail::to_conn(arg), a);
			jack_port_t* port_b = jack_port_by_id(detail::to_conn(arg), b);

			const char* port_name_a = jack_port_name(port_a);
			const char* port_name_b = jack_port_name(port_b);

			PACEMAKER_LOG(LogLevel::WRN, port_name_a, " is ", states.at(is_connecting), " ", port_name_b);
		}

		void port_registration_callback(jack_port_id_t port_id, int is_registering, void* arg) {
			constexpr std::array states { "unregistering", "registering" };

			jack_port_t* port = jack_port_by_id(detail::to_conn(arg), port_id);
			const char* port_name = jack_port_name(port);

			PACEMAKER_LOG(LogLevel::WRN, port_name, " is ", states.at(is_registering));
		}

		void port_rename_callback(jack_port_id_t port, const char* old_name, const char* new_name, void*) {
			PACEMAKER_LOG(LogLevel::WRN, port, " is renaming from ", old_name, " to ", new_name);
		}

		int xrun_callback(void* arg) {
			float usecs = jack_get_xrun_delayed_usecs(detail::to_conn(arg));
			PACEMAKER_LOG(LogLevel::WRN, "xrun occured with delay of ", usecs, "μs");
			return 0;
		}
	}  // namespace detail
}  // namespace pacemaker

#endif
