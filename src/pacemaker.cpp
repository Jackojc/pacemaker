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

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
	try {
		pacemaker::JackClient client;

		auto port = client.port_register_output("port1");
		port.connect("Midi-Bridge:NTS-1");

		client.set_callback([&](jack_nframes_t frames, pacemaker::JackClient&) {
			PACEMAKER_LOG(pacemaker::LogLevel::OK, frames);
			return 0;
		});

		client.ready();

		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds { 1 });
		}

		// JackPorts ports { jack_get_ports(
		// 	data.client, "", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput) };

		// if (jack_connect(data.client, jack_port_name(data.port), ports[0]))
		// {}
	}

	catch (pacemaker::Fatal) {
		pacemaker::error("fatal error");
		return 1;
	}

	return 0;
}
