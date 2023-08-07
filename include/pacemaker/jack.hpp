#ifndef PACEMAKER_JACK_HPP
#define PACEMAKER_JACK_HPP

#include <array>
#include <vector>
#include <list>
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
		inline int process_callback(jack_nframes_t, void*);

		inline int sample_rate_callback(jack_nframes_t, void*);
		inline int buffer_size_callback(jack_nframes_t, void*);

		inline void client_registration_callback(const char*, int, void*);

		inline void port_connect_callback(jack_port_id_t, jack_port_id_t, int, void*);
		inline void port_registration_callback(jack_port_id_t, int, void*);
		inline void port_rename_callback(jack_port_id_t, const char*, const char*, void*);

		inline int xrun_callback(void*);

		// Cast void* argument to JackClient.
		inline JackClient& to_conn(void* arg) {
			return *static_cast<JackClient*>(arg);
		}
	}  // namespace detail

	namespace detail {
		struct JackPortConnectionsDeleter {
			void operator()(const char** connections) {
				jack_free(connections);
			}
		};

		struct JackRingBufferDeleter {
			void operator()(jack_ringbuffer_t* rb) {
				jack_ringbuffer_free(rb);
			}
		};
	}  // namespace detail

	using JackPortConnections = std::unique_ptr<const char*[], detail::JackPortConnectionsDeleter>;

	namespace detail {
		template <typename T1, typename T2, typename F, typename... Ts>
		inline decltype(auto) null_array_to_vec(F&& fn, Ts&&... args) {
			struct NullArrayDeleter {
				void operator()(T1 arr[]) {
					jack_free(arr);
				}
			};

			using NullArray = std::unique_ptr<T1[], NullArrayDeleter>;

			const NullArray arr { fn(std::forward<Ts>(args)...) };
			std::vector<T2> out;

			if (not arr) {
				return out;
			}

			for (size_t i = 0; arr[i] != nullptr; ++i) {
				out.emplace_back(arr[i]);
			}

			return out;
		}
	}  // namespace detail

	struct JackClient;
	struct JackPort;

	struct JackPort {
		JackClient* client;
		jack_port_t* port;

		jack_ringbuffer_t* buffer;

		operator jack_port_t*() const {
			return port;
		}

		jack_port_t* get() const {
			return port;
		}

		JackPort(JackClient* client_, jack_port_t* port_):
				client(client_), port(port_), buffer(jack_ringbuffer_create(RINGBUFFER_SIZE)) {}

		~JackPort();

		JackPort(JackPort&& other) noexcept:
				client(std::exchange(other.client, nullptr)),
				port(std::exchange(other.port, nullptr)),
				buffer(std::exchange(other.buffer, nullptr)) {}

		JackPort& operator=(JackPort&& other) noexcept {
			std::swap(client, other.client);
			std::swap(port, other.port);
			std::swap(buffer, other.buffer);

			return *this;
		}

		bool connect(const std::string& dst) const;
		bool disconnect(const std::string& dst) const;
		bool rename(const std::string& name) const;

		int connected() const;
		std::vector<std::string> get_connections() const;

		void* get_buffer(jack_nframes_t frames) const;

		bool send(const char* data, size_t count);
	};

	namespace detail {
		template <typename... Ts>
		inline bool check_fatal(jack_status_t status, Ts&&... args) {
			return ([&](auto&& p) {
				auto&& [flag, str] = p;
				bool is_present = (status & flag) == flag;

				if (is_present) {
					pacemaker::fatal_error(str);
				}

				return is_present;
			}(std::forward<Ts>(args)) and
				...);
		}

		template <typename... Ts>
		inline bool check_warning(jack_status_t status, Ts&&... args) {
			return ([&](auto&& p) {
				auto&& [flag, str] = p;
				bool is_present = (status & flag) == flag;

				if (is_present) {
					pacemaker::warning(str);
				}

				return is_present;
			}(std::forward<Ts>(args)) and
				...);
		}
	}  // namespace detail

	struct JackClient {
		std::list<JackPort> ports;

		jack_client_t* client;

		jack_nframes_t sample_rate;
		jack_nframes_t buffer_size;

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
				pacemaker::fatal_error(STR_ERROR_CLIENT);
			}

			detail::check_fatal(flags,
				std::pair { JackFailure, STR_ERROR_GENERAL },
				std::pair { JackInvalidOption, STR_ERROR_INVALID_OPTION },
				std::pair { JackNameNotUnique, STR_ERROR_NAME_EXISTS },
				std::pair { JackServerStarted, STR_WARNING_STARTED },
				std::pair { JackServerFailed, STR_ERROR_CONNECT },
				std::pair { JackServerError, STR_ERROR_COMMUNICATION },
				std::pair { JackNoSuchClient, STR_ERROR_NO_CLIENT },
				std::pair { JackLoadFailure, STR_ERROR_LOAD_INTERNAL_CLIENT },
				std::pair { JackShmFailure, STR_ERROR_INITIALISATION },
				std::pair { JackVersionError, STR_ERROR_PROTOCOL },
				std::pair { JackBackendError, STR_ERROR_INTERNAL },
				std::pair { JackClientZombie, STR_ERROR_ZOMBIE });

			detail::check_warning(flags, std::pair { JackServerStarted, STR_WARNING_STARTED });

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

			// Callback for reading/writing data from/to ports.
			if (PACEMAKER_DBG(jack_set_process_callback(client, detail::process_callback, static_cast<void*>(this)))) {
				pacemaker::fatal_error("could not set process callback");
			}

			buffer_size = PACEMAKER_DBG(jack_get_buffer_size(client));
			sample_rate = PACEMAKER_DBG(jack_get_sample_rate(client));
		}

		JackClient(std::list<JackPort>&& ports_,
			jack_client_t* client_,
			jack_nframes_t sample_rate_,
			jack_nframes_t buffer_size_):
				ports(std::move(ports_)), client(client_), sample_rate(sample_rate_), buffer_size(buffer_size_) {}

		~JackClient() {
			PACEMAKER_DBG(jack_deactivate(client));
			PACEMAKER_DBG(jack_client_close(client));
		}

		JackClient(JackClient&& other) noexcept:
				ports(std::exchange(other.ports, {})),
				client(std::exchange(other.client, nullptr)),
				sample_rate(std::exchange(other.sample_rate, 0)),
				buffer_size(std::exchange(other.buffer_size, 0)) {}

		JackClient& operator=(const JackClient& other) = delete;

		JackClient& operator=(JackClient&& other) noexcept {
			std::swap(ports, other.ports);

			std::swap(client, other.client);

			std::swap(sample_rate, other.sample_rate);
			std::swap(buffer_size, other.buffer_size);

			return *this;
		}

		JackPort& port_register_output(const std::string& name) {
			return ports.emplace_back(
				this, jack_port_register(client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0));
		}

		JackPort& port_register_input(const std::string& name) {
			return ports.emplace_back(
				this, jack_port_register(client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
		}

		bool port_is_mine(const JackPort& port) const {
			return jack_port_is_mine(client, port);
		}

		// Start processing MIDI (activates the user callback).
		bool ready() const {
			bool is_fail = PACEMAKER_DBG(jack_activate(client));
			return not(is_fail);
		}

		// TODO: List all ports
		std::vector<std::string> get_ports(const std::string& name = "") const {
			return detail::null_array_to_vec<const char*, std::string>(
				jack_get_ports, client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, 0);
		}

		std::vector<std::string> get_input_ports(const std::string& name = "") const {
			return detail::null_array_to_vec<const char*, std::string>(
				jack_get_ports, client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
		}

		std::vector<std::string> get_output_ports(const std::string& name = "") const {
			return detail::null_array_to_vec<const char*, std::string>(
				jack_get_ports, client, name.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
		}

		std::vector<std::string> get_my_ports() const {
			return detail::null_array_to_vec<const char*, std::string>(
				jack_get_ports, client, STR_CLIENT_NAME, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
		}
	};

	// JackPort member function definitions
	JackPort::~JackPort() {
		jack_ringbuffer_free(buffer);
		PACEMAKER_DBG(jack_port_unregister(client->get(), port));
	}

	bool JackPort::connect(const std::string& dst) const {
		bool is_fail = PACEMAKER_DBG(jack_connect(client->get(), jack_port_name(port), dst.c_str()));
		return not(is_fail);
	}

	bool JackPort::disconnect(const std::string& dst) const {
		bool is_fail = PACEMAKER_DBG(jack_disconnect(client->get(), jack_port_name(port), dst.c_str()));
		return not(is_fail);
	}

	bool JackPort::rename(const std::string& name) const {
		bool is_fail = jack_port_rename(client->get(), port, name.c_str());
		return not(is_fail);
	}

	int JackPort::connected() const {
		return jack_port_connected(port);
	}

	std::vector<std::string> JackPort::get_connections() const {
		return detail::null_array_to_vec<const char*, std::string>(jack_port_get_connections, port);
	}

	void* JackPort::get_buffer(jack_nframes_t frames) const {
		return jack_port_get_buffer(port, frames);
	}

	bool JackPort::send(const char* data, size_t count) {
		if (jack_ringbuffer_write_space(buffer) < count) {
			return false;
		}

		return count == jack_ringbuffer_write(buffer, data, count);
	}

	// Callbacks
	namespace detail {
		inline int process_callback(jack_nframes_t nframes, void* arg) {
			auto& client = detail::to_conn(arg);
			auto& ports = client.ports;

			for (auto& port: ports) {
				void* buffer = port.get_buffer(nframes);
				jack_midi_clear_buffer(buffer);

				size_t count = jack_ringbuffer_read_space(port.buffer);

				if (not count) {  // No events.
					return 0;
				}

				jack_midi_data_t* midi = jack_midi_event_reserve(buffer, 0, count);

				if (not midi) {
					return 1;
				}

				// This will automatically advance the internal data pointer of the ringbuffer and we don't need to
				// handle the case of data wrapping across the end boundary.
				size_t count_copied = jack_ringbuffer_read(port.buffer, reinterpret_cast<char*>(midi), count);

				if (count_copied != count) {
					return 1;
				}
			}

			return 0;
		}

		inline int sample_rate_callback(jack_nframes_t new_sample_rate, void* arg) {
			PACEMAKER_LOG(LogLevel::WRN, "sample rate changed");
			detail::to_conn(arg).sample_rate = new_sample_rate;
			return 0;
		}

		inline int buffer_size_callback(jack_nframes_t new_buffer_size, void* arg) {
			PACEMAKER_LOG(LogLevel::WRN, "buffer size changed");
			detail::to_conn(arg).buffer_size = new_buffer_size;
			return 0;
		}

		inline void client_registration_callback(const char* client, int is_registering, void*) {
			constexpr std::array states { "unregistering", "registering" };
			PACEMAKER_LOG(LogLevel::WRN, client, " is ", states.at(is_registering));
		}

		inline void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int is_connecting, void* arg) {
			constexpr std::array states { "disconnecting from", "connecting to" };

			jack_port_t* port_a = jack_port_by_id(detail::to_conn(arg), a);
			jack_port_t* port_b = jack_port_by_id(detail::to_conn(arg), b);

			const char* port_name_a = jack_port_name(port_a);
			const char* port_name_b = jack_port_name(port_b);

			PACEMAKER_LOG(LogLevel::WRN, port_name_a, " is ", states.at(is_connecting), " ", port_name_b);
		}

		inline void port_registration_callback(jack_port_id_t port_id, int is_registering, void* arg) {
			// TODO: Check if any of our known ports unregistered.

			constexpr std::array states { "unregistering", "registering" };

			jack_port_t* port = jack_port_by_id(detail::to_conn(arg), port_id);
			const char* port_name = jack_port_name(port);

			PACEMAKER_LOG(LogLevel::WRN, port_name, " is ", states.at(is_registering));
		}

		inline void port_rename_callback(jack_port_id_t port, const char* old_name, const char* new_name, void*) {
			PACEMAKER_LOG(LogLevel::WRN, port, " is renaming from ", old_name, " to ", new_name);
		}

		inline int xrun_callback(void* arg) {
			float usecs = jack_get_xrun_delayed_usecs(detail::to_conn(arg));
			PACEMAKER_LOG(LogLevel::WRN, "xrun occured with delay of ", usecs, "μs");
			return 0;
		}
	}  // namespace detail
}  // namespace pacemaker

#endif
