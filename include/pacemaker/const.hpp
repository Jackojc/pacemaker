#ifndef PACEMAKER_CONST_HPP
#define PACEMAKER_CONST_HPP

// Misc. Constants
namespace pacemaker {
	constexpr auto RINGBUFFER_SIZE = 16'384;
}

// Strings
namespace pacemaker {
	constexpr auto STR_CLIENT_NAME = "pacemaker";

	constexpr auto STR_WARNING_STARTED = "JACK server was started";

	constexpr auto STR_ERROR_CLIENT = "could not open client";
	constexpr auto STR_ERROR_GENERAL = "general error occured";
	constexpr auto STR_ERROR_INVALID_OPTION = "invalid or unsupported option";
	constexpr auto STR_ERROR_NAME_EXISTS = "client name is not unique";
	constexpr auto STR_ERROR_CONNECT = "cannot connect to the JACK server";
	constexpr auto STR_ERROR_COMMUNICATION = "communication error with JACK server";
	constexpr auto STR_ERROR_NO_CLIENT = "requested client does not exist";
	constexpr auto STR_ERROR_LOAD_INTERNAL_CLIENT = "could not load internal client";
	constexpr auto STR_ERROR_INITIALISATION = "initialisation failed";
	constexpr auto STR_ERROR_PROTOCOL = "protocol version mismatch";
	constexpr auto STR_ERROR_INTERNAL = "internal error occured";
	constexpr auto STR_ERROR_ZOMBIE = "zombie client";
}  // namespace pacemaker

// MIDI Constants
namespace pacemaker {
	constexpr auto MIDI_NOTE_OFF = 0b1000'0000;
	constexpr auto MIDI_NOTE_ON = 0b1001'0000;
}

#endif