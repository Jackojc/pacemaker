#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include <conflict/conflict.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <pacemaker/pacemaker.hpp>

int main(int argc, const char* argv[]) {
	try {
		pacemaker::JackConnection conn {};

		// if (jack_set_process_callback(
		// 		data.client,
		// 		[](jack_nframes_t nframes, void* arg) {
		// 			auto& data = *static_cast<pacemaker::JackData*>(arg);

		// 			void* out_buffer = jack_port_get_buffer(data.port, nframes);
		// 			jack_midi_clear_buffer(out_buffer);

		// 			// Copy every MIDI event into the buffer provided by JACK.
		// 			// for (; it != end and it->time <= time; ++it) {
		// 			// 	if (jack_midi_event_write(
		// 			// 			out_buffer,
		// 			// 			0,
		// 			// 			it->data.data(),
		// 			// 			it->data.size())) {
		// 			// 		cane::general_error(cane::STR_WRITE_ERROR);
		// 			// 	}
		// 			// }

		// 			// size_t lost = 0;
		// 			// if ((lost = jack_midi_get_lost_event_count(out_buffer)))
		// 			// { 	cane::general_warning(cane::STR_LOST_EVENT, lost);
		// 			// }

		// 			// time += cane::SECOND / (sample_rate / nframes);

		// 			return 0;
		// 		},
		// 		static_cast<void*>(&data))) {
		// 	PACEMAKER_LOG(
		// 		pacemaker::LogLevel::ERR, "could not set process callback");
		// }

		// if (not(data.port = jack_port_register(
		// 			data.client,
		// 			"port_1",
		// 			JACK_DEFAULT_MIDI_TYPE,
		// 			JackPortIsOutput,
		// 			0))) {
		// 	PACEMAKER_LOG(pacemaker::LogLevel::ERR, "could not register port");
		// }

		// if (not(data.port = jack_port_register(
		// 			data.client,
		// 			"port_2",
		// 			JACK_DEFAULT_MIDI_TYPE,
		// 			JackPortIsOutput,
		// 			0))) {
		// 	PACEMAKER_LOG(pacemaker::LogLevel::ERR, "could not register port");
		// }

		// data.buffer_size = jack_get_buffer_size(data.client);
		// data.sample_rate = jack_get_sample_rate(data.client);

		// JackPorts ports { jack_get_ports(
		// 	data.client, "", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput) };

		// if (jack_connect(data.client, jack_port_name(data.port), ports[0]))
		// {}

		std::this_thread::sleep_for(std::chrono::seconds { 10 });

	}

	catch (pacemaker::Fatal) {
		PACEMAKER_LOG(pacemaker::LogLevel::ERR, "fatal error");
		return 1;
	}

	return 0;
}
