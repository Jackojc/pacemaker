#ifndef PACEMAKER_SEQUENCER_HPP
#define PACEMAKER_SEQUENCER_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <chrono>
#include <algorithm>

#include <cmath>

namespace pacemaker {
	using Unit = std::chrono::microseconds;

	using MidiPrimitive = uint8_t;
	using MidiStatus = MidiPrimitive;
	using MidiFunction = MidiPrimitive;
	using MidiChannel = MidiPrimitive;
	using MidiNote = MidiPrimitive;
	using MidiVelocity = MidiPrimitive;
	using MidiData = MidiPrimitive;

	struct Status {
		MidiChannel channel;
		MidiFunction function;
	};

	using Notes = std::vector<MidiNote>;

	struct Channel {
		Status status;

		pacemaker::Unit frequency;
		pacemaker::Unit offset;

		pacemaker::Notes notes;

		Channel() = default;

		Channel(Status status_, pacemaker::Unit frequency_, pacemaker::Unit offset_, pacemaker::Notes notes_):
				status(status_), frequency(frequency_), offset(offset_), notes(notes_) {}
	};

	using Patch = std::vector<pacemaker::Channel>;

	using Midi = std::array<MidiPrimitive, 3>;

	struct Event {
		pacemaker::Unit timestamp;
		pacemaker::Midi midi;

		Event() = default;

		Event(pacemaker::Unit timestamp_, pacemaker::Midi midi_): timestamp(timestamp_), midi(midi_) {}

		auto operator<=>(const Event&) const = default;
	};

	using Timeline = std::vector<pacemaker::Event>;

	namespace detail {
		using namespace std::literals;

		Unit event_at(Unit timestamp, Unit frequency, Unit offset = 0s) {
			timestamp += offset;

			return std::chrono::duration_cast<Unit>(frequency *
				std::ceil(std::chrono::duration<double>(timestamp).count() /
					std::chrono::duration<double>(frequency).count()));
		}

		size_t events_between(Unit begin, Unit end, Unit frequency, Unit offset = 0s) {
			begin += offset;
			end += offset;

			return (end - begin) / frequency;
		}

		size_t events_until(Unit timestamp, Unit frequency, Unit offset = 0s) {
			return events_between(offset, timestamp, frequency, offset);
		}
	}  // namespace detail

	pacemaker::Timeline timeline(pacemaker::Unit begin, pacemaker::Unit end, const pacemaker::Patch& p) {
		using namespace std::literals;

		pacemaker::Timeline tl;

		for (auto it = p.begin(); it != p.end(); ++it) {
			auto& [status, frequency, offset, notes] = *it;

			// Find the extent of the events we need for this slice of time.
			auto first_event = detail::event_at(begin, frequency, offset);
			auto last_event = detail::event_at(end, frequency, offset);

			// Generate all of the in-between events (exclusive).
			size_t event_count = detail::events_between(first_event, last_event, frequency, offset);
			size_t n_before = detail::events_until(first_event, frequency, offset);

			for (size_t i = 0; i != event_count; ++i) {
				pacemaker::Unit timestamp = frequency * i + first_event + offset;

				// if (timestamp >= end or timestamp < begin) {
				// 	PACEMAKER_LOG(LogLevel::ERR, timestamp, " >= ", end, " or ", timestamp, " < ", begin);
				// }

				pacemaker::MidiNote note = notes.at((i + n_before) % notes.size());
				pacemaker::MidiVelocity velocity = 127;

				MidiStatus midi_status = status.channel | status.function;

				tl.emplace_back(timestamp, pacemaker::Midi { midi_status, note, velocity });
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

	inline std::ostream& operator<<(std::ostream& os, const Status& ch) {
		return (os << "{channel: " << ch.channel << ", function: " << ch.function << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Channel& ch) {
		return (os << "{status: " << ch.status << ", frequency: " << ch.frequency << ", notes: " << ch.notes << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Patch& p) {
		return detail::serialise_container(os, p);
	}

	inline std::ostream& operator<<(std::ostream& os, const Midi& m) {
		return (os << "{status: " << (int)m.at(0) << ", data1: " << (int)m.at(1) << ", data2: " << (int)m.at(2) << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Event& ev) {
		return (os << "{timestamp: " << ev.timestamp << ", data: " << ev.midi << "}");
	}

	inline std::ostream& operator<<(std::ostream& os, const Timeline& tl) {
		return detail::serialise_container(os, tl);
	}
}  // namespace pacemaker

#endif