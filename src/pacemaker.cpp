#include "pacemaker/const.hpp"
#include "pacemaker/sequencer.hpp"
#include "pacemaker/util.hpp"
#include <bits/chrono.h>
#include <exception>
#include <jack/jack.h>
#include <jack/types.h>
#include <utility>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <atomic>
#include <thread>
#include <vector>
#include <compare>

#include <cmath>
#include <cstdint>

#include <conflict/conflict.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <pacemaker/pacemaker.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
	try {
		using namespace std::literals;

		PACEMAKER_LOG(pacemaker::LogLevel::OK, "starting");

		pacemaker::JackClient client;
		auto& port = client.port_register_output(pacemaker::STR_CLIENT_NAME);

		PACEMAKER_LOG(pacemaker::LogLevel::OK, "collecting ports");

		PACEMAKER_ASSERT(argc > 1);
		auto ports = client.get_input_ports(argv[1]);

		pacemaker::println(std::cerr, "matches:");
		for (auto& p: ports) {
			pacemaker::println(std::cerr, p);
		}

		PACEMAKER_ASSERT(port.connect(ports.front()));
		PACEMAKER_ASSERT(client.ready());

		PACEMAKER_LOG(pacemaker::LogLevel::OK, "ready");

		std::this_thread::sleep_for(1s);

		// pacemaker::Unit buffer_size = 5s;
		// auto default_patch = pacemaker::Patch {
		// 	pacemaker::Channel { { 0, pacemaker::MIDI_NOTE_ON }, 2s, 0s, pacemaker::Notes { 64 } },
		// 	pacemaker::Channel { { 0, pacemaker::MIDI_NOTE_OFF }, 2s, 1s, pacemaker::Notes { 64 } },
		// };

		// pacemaker::Timeline timeline_reader;
		// pacemaker::Timeline::iterator it;

		// std::atomic_bool writer_ready = false;
		// std::atomic_bool reader_ready = false;

		// client.set_callback([&](jack_nframes_t frames) {
		// 	using namespace std::literals;
		// 	using namespace pacemaker;

		// 	void* buffer = port.get_buffer(frames);
		// 	jack_midi_clear_buffer(buffer);

		// 	if (writer_ready) {
		// 		return 0;
		// 	}

		// 	jack_nframes_t current_frames;
		// 	jack_time_t current_usecs;
		// 	jack_time_t next_usecs;
		// 	float period_usecs;

		// 	jack_get_cycle_times(client, &current_frames, &current_usecs, &next_usecs, &period_usecs);

		// 	Unit current = std::chrono::duration_cast<Unit>(std::chrono::microseconds { current_usecs });

		// 	for (; it != timeline_reader.end() and it->timestamp <= current; ++it) {
		// 		jack_midi_event_write(buffer, 0, it->midi.data(), it->midi.size());
		// 	}

		// 	if (it == timeline_reader.end()) {
		// 		reader_ready = true;
		// 	}

		// 	return 0;
		// });

		// PACEMAKER_ASSERT(client.ready());
		// PACEMAKER_ASSERT(port.connect("lmms:MIDI in"));

		// PACEMAKER_LOG(pacemaker::LogLevel::OK, "ready");

		// while (true) {
		// 	jack_nframes_t current_frames = jack_frame_time(client);
		// 	jack_time_t current_time = jack_frames_to_time(client, current_frames);

		// 	auto current = std::chrono::duration_cast<pacemaker::Unit>(std::chrono::microseconds { current_time });

		// 	auto begin = pacemaker::detail::event_at(current + buffer_size, buffer_size);
		// 	auto end = begin + buffer_size;

		// 	auto timeline_writer = pacemaker::timeline(begin, end, default_patch);

		// 	PACEMAKER_LOG(pacemaker::LogLevel::OK, "buffer generated! ", begin, " -> ", end);
		// 	writer_ready = true;

		// 	while (not reader_ready) {}

		// 	std::swap(timeline_reader, timeline_writer);
		// 	it = timeline_reader.begin();

		// 	writer_ready = false;
		// 	reader_ready = false;
		// }

		PACEMAKER_LOG(pacemaker::LogLevel::OK, "done!");
	}

	catch (pacemaker::Fatal) {
		pacemaker::error("fatal error");
		return 1;
	}

	return 0;
}
