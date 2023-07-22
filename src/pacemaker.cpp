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

		auto port = client.port_register_output("foo");
		// auto port2 = client.port_register_output("foo2");

		client.set_callback([&](jack_nframes_t) {
			// PACEMAKER_LOG(pacemaker::LogLevel::OK, frames);
			// TODO: Calculate time passed.
			return 0;
		});

		PACEMAKER_ASSERT(client.ready());
		// PACEMAKER_ASSERT(port.connect("Midi-Bridge:Midi Through:(playback_0) Midi Through Port-0"));

		// size_t i = 0;
		// std::vector<pacemaker::JackPort> ports;

		// auto ports = client.get_my_ports();
		// for (auto& p: ports) {
		// 	pacemaker::println(std::cerr, "Mine: ", p);
		// }

		// while (true) {
		// 	// pacemaker::println(std::cerr, port_a.get_connections().size());
		// 	// ports.emplace_back(client.port_register_output(std::to_string(i++)));
		// 	auto ports = port.get_connections();
		// 	for (auto& p: ports) {
		// 		pacemaker::println(std::cerr, p);
		// 	}
		// 	std::this_thread::sleep_for(std::chrono::seconds { 1 });
		// }

		// std::this_thread::sleep_for(std::chrono::seconds { 1 });

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
