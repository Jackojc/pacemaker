#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
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

namespace pacemaker {
	using Unit = std::chrono::milliseconds;

	using MidiPrimitive = uint8_t;
	using MidiStatus = MidiPrimitive;
	using MidiFunction = MidiPrimitive;
	using MidiChannel = MidiPrimitive;
	using MidiNote = MidiPrimitive;
	using MidiVelocity = MidiPrimitive;
	using MidiData = MidiPrimitive;

	using Notes = std::vector<MidiNote>;

	struct Channel {
		MidiChannel channel;
		pacemaker::Unit frequency;
		pacemaker::Notes notes;

		Channel() = default;

		Channel(MidiChannel channel_, pacemaker::Unit frequency_, pacemaker::Notes notes_):
				channel(channel_), frequency(frequency_), notes(notes_) {}
	};

	using Patch = std::vector<pacemaker::Channel>;

	using Midi = std::array<MidiPrimitive, 3>;

	struct Event {
		pacemaker::Unit timestamp;
		pacemaker::Midi data;

		Event() = default;

		Event(pacemaker::Unit timestamp_, pacemaker::Midi data_): timestamp(timestamp_), data(data_) {}

		auto operator<=>(const Event&) const = default;
	};

	using Timeline = std::vector<pacemaker::Event>;

	namespace detail {
		pacemaker::Unit event_at(pacemaker::Unit timestamp, pacemaker::Unit frequency) {
			return std::chrono::duration_cast<pacemaker::Unit>(frequency *
				std::ceil(std::chrono::duration<double>(timestamp).count() /
					std::chrono::duration<double>(frequency).count()));
		}

		size_t events_between(pacemaker::Unit begin, pacemaker::Unit end, pacemaker::Unit frequency) {
			return (end - begin) / frequency;
		}
	}  // namespace detail

	pacemaker::Timeline timeline(pacemaker::Unit begin, pacemaker::Unit end, const pacemaker::Patch& p) {
		using namespace std::literals;

		pacemaker::Timeline tl;

		for (auto it = p.begin(); it != p.end(); ++it) {
			auto& [channel, frequency, notes] = *it;

			// Find the extent of the events we need for this slice of time.
			auto first_event = PACEMAKER_DBG(detail::event_at(begin, frequency));
			auto last_event = PACEMAKER_DBG(detail::event_at(end, frequency));

			// Generate all of the in-between events (exclusive).
			size_t event_count = PACEMAKER_DBG(detail::events_between(first_event, last_event, frequency));
			size_t offset = PACEMAKER_DBG(detail::events_between(0ms, first_event, frequency));

			for (size_t i = 0; i != event_count; ++i) {
				pacemaker::Unit timestamp = PACEMAKER_DBG(frequency * i + (first_event));

				if (timestamp >= end or timestamp < begin) {
					PACEMAKER_LOG(LogLevel::ERR, timestamp, " >= ", end, " or ", timestamp, " < ", begin);
				}

				pacemaker::MidiStatus status = static_cast<MidiStatus>(0b1000'0000 | channel);
				pacemaker::MidiNote note = notes.at((i + offset) % notes.size());
				pacemaker::MidiVelocity velocity = 127;

				tl.emplace_back(timestamp, pacemaker::Midi { status, note, velocity });
			}
		}

		std::sort(tl.begin(), tl.end());

		return tl;
	}
}  // namespace pacemaker

// std::ostream overloads
namespace pacemaker {
	namespace detail {
		template <typename T>
		inline std::ostream& serialise_container(std::ostream& os, const T& c) {
			if (c.empty()) {
				return (os << "[]");
			}

			auto it = c.begin();
			os << '[' << *it;

			for (; it != c.end(); ++it) {
				os << ", " << *it;
			}

			return (os << ']');
		}
	}  // namespace detail

	inline std::ostream& operator<<(std::ostream& os, const Notes& ns) {
		return detail::serialise_container(os, ns);
	}

	inline std::ostream& operator<<(std::ostream& os, const Channel& ch) {
		return (os << "{channel: " << ch.channel << ", frequency: " << ch.frequency << ", notes: " << ch.notes << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Patch& p) {
		return detail::serialise_container(os, p);
	}

	inline std::ostream& operator<<(std::ostream& os, const Midi& m) {
		return (os << "{status: " << (int)m.at(0) << ", data1: " << (int)m.at(1) << ", data2: " << (int)m.at(2) << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Event& ev) {
		return (os << "{timestamp: " << ev.timestamp << ", data: " << ev.data << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Timeline& tl) {
		return detail::serialise_container(os, tl);
	}
}  // namespace pacemaker

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
	try {
		using namespace std::literals;

		auto patch = pacemaker::Patch {
			pacemaker::Channel { 1, 100ms, pacemaker::Notes { 54, 64, 74 } },
			pacemaker::Channel { 2, 175ms, pacemaker::Notes { 60 } },
			pacemaker::Channel { 3, 200ms, pacemaker::Notes { 50, 57 } },
		};

		auto tl = pacemaker::timeline(150ms, 1000ms, patch);
		pacemaker::println(std::cerr, tl);

		for (pacemaker::Event ev: tl) {
			pacemaker::println(std::cerr, ev);
		}

		// pacemaker::println(
		// 	std::cerr,
		// 	std::chrono::duration_cast<std::chrono::milliseconds>(pacemaker::detail::nearest_event(400ms, 200ms)));

		// pacemaker::JackClient client;

		// auto port = client.port_register_output("foo");
		// auto port2 = client.port_register_output("foo2");

		// client.set_callback([&](jack_nframes_t) {
		// PACEMAKER_LOG(pacemaker::LogLevel::OK, frames);
		// TODO: Calculate time passed.
		// return 0;
		// });

		// PACEMAKER_ASSERT(client.ready());
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
