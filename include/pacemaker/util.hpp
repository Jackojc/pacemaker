#ifndef PACEMAKER_UTIL_HPP
#define PACEMAKER_UTIL_HPP

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

// Macros
namespace pacemaker {
#define PACEMAKER_STR_IMPL_(x) #x
#define PACEMAKER_STR(x) PACEMAKER_STR_IMPL_(x)

#define PACEMAKER_CAT_IMPL_(x, y) x##y
#define PACEMAKER_CAT(x, y) PACEMAKER_CAT_IMPL_(x, y)

#define PACEMAKER_VAR(x) PACEMAKER_CAT(var_, PACEMAKER_CAT(x, PACEMAKER_CAT(__LINE__, _)))

#define PACEMAKER_TRACE "[" __FILE__ ":" PACEMAKER_STR(__LINE__) "] "
}  // namespace pacemaker

// I/O
namespace pacemaker {
	namespace detail {
		template <typename... Ts> inline std::ostream& print_impl(std::ostream& os, Ts&&... xs) {
			return ((os << std::forward<Ts>(xs)), ..., os);
		}

		template <typename... Ts> inline std::ostream& println_impl(std::ostream& os, Ts&&... xs) {
			return ((os << std::forward<Ts>(xs)), ..., (os << '\n'));
		}
	}  // namespace detail

	template <typename T, typename... Ts> inline std::ostream& print(std::ostream& os, T&& x, Ts&&... xs) {
		return detail::print_impl(os, std::forward<T>(x), std::forward<Ts>(xs)...);
	}

	template <typename T, typename... Ts> inline std::ostream& println(std::ostream& os, T&& x, Ts&&... xs) {
		return detail::println_impl(os, std::forward<T>(x), std::forward<Ts>(xs)...);
	}
}  // namespace pacemaker

// Logging
namespace pacemaker {
#ifndef PACEMAKER_ANSI_DISABLE
	#define PACEMAKER_RESET "\x1b[0m"
	#define PACEMAKER_BOLD "\x1b[1m"

	#define PACEMAKER_BLACK "\x1b[30m"
	#define PACEMAKER_RED "\x1b[31m"
	#define PACEMAKER_GREEN "\x1b[32m"
	#define PACEMAKER_YELLOW "\x1b[33m"
	#define PACEMAKER_BLUE "\x1b[34m"
	#define PACEMAKER_MAGENTA "\x1b[35m"
	#define PACEMAKER_CYAN "\x1b[36m"
	#define PACEMAKER_WHITE "\x1b[37m"

	#define PACEMAKER_INF PACEMAKER_RESET PACEMAKER_BOLD
	#define PACEMAKER_WRN PACEMAKER_BLUE PACEMAKER_BOLD
	#define PACEMAKER_ERR PACEMAKER_RED PACEMAKER_BOLD
	#define PACEMAKER_OK PACEMAKER_GREEN PACEMAKER_BOLD
#else
	#define PACEMAKER_RESET ""
	#define PACEMAKER_BOLD ""

	#define PACEMAKER_BLACK ""
	#define PACEMAKER_RED ""
	#define PACEMAKER_GREEN ""
	#define PACEMAKER_YELLOW ""
	#define PACEMAKER_BLUE ""
	#define PACEMAKER_MAGENTA ""
	#define PACEMAKER_CYAN ""
	#define PACEMAKER_WHITE ""

	#define PACEMAKER_INF ""
	#define PACEMAKER_WRN ""
	#define PACEMAKER_ERR ""
	#define PACEMAKER_OK ""
#endif

	struct Fatal {};

#ifndef NDEBUG
	namespace detail {
		template <typename T>
		inline decltype(auto) dbg_impl(const char* file, const char* line, const char* expr_s, T&& expr) {
			println(
				std::cerr,
				"[",
				std::filesystem::relative(file).native(),
				":",
				line,
				"] ",
				expr_s,
				" = ",
				std::forward<T>(expr));

			return std::forward<T>(expr);
		}
	}  // namespace detail

	#define PACEMAKER_DBG(expr) \
		(pacemaker::detail::dbg_impl(__FILE__, PACEMAKER_STR(__LINE__), PACEMAKER_STR(expr), (expr)))

	#define PACEMAKER_DBG_RUN(expr) \
		do {                        \
			((expr));               \
		} while (0)
#else
	#define PACEMAKER_DBG(expr) ((expr))
	#define PACEMAKER_DBG_RUN(expr) \
		do {                        \
		} while (0)
#endif

#define LOG_LEVELS                \
	X(INF, PACEMAKER_RESET "[-]") \
	X(WRN, PACEMAKER_BLUE "[*]")  \
	X(ERR, PACEMAKER_RED "[!]")   \
	X(OK, PACEMAKER_GREEN "[^]")

#define X(a, b) a,
	enum class LogLevel : size_t {
		LOG_LEVELS
	};
#undef X

	namespace detail {
#define X(a, b) std::string_view { b },
		constexpr std::array LOG_TO_STR = { LOG_LEVELS };
#undef X

		constexpr std::string_view log_to_str(LogLevel x) {
			return LOG_TO_STR[static_cast<size_t>(x)];
		}

		constexpr size_t log_padding() {
			return std::max_element(detail::LOG_TO_STR.begin(), detail::LOG_TO_STR.end())->size();
		}
	}  // namespace detail

	inline std::ostream& operator<<(std::ostream& os, LogLevel x) {
		return print(os, detail::log_to_str(x));
	}

#define PACEMAKER_LOG(...)                                                                                         \
	do {                                                                                                           \
		[PACEMAKER_VAR(fn_name) = __func__](pacemaker::LogLevel PACEMAKER_VAR(x), auto&&... PACEMAKER_VAR(args)) { \
			using namespace std::string_view_literals;                                                             \
                                                                                                                   \
			PACEMAKER_DBG_RUN((pacemaker::print(std::cerr, PACEMAKER_VAR(x), " ", PACEMAKER_TRACE)));              \
                                                                                                                   \
			if (PACEMAKER_VAR(fn_name) != "operator()"sv) {                                                        \
				PACEMAKER_DBG_RUN((pacemaker::print(std::cerr, "`", PACEMAKER_VAR(fn_name), "` ")));               \
			}                                                                                                      \
                                                                                                                   \
			if constexpr (sizeof...(PACEMAKER_VAR(args)) > 0) {                                                    \
				PACEMAKER_DBG_RUN((pacemaker::print(                                                               \
					std::cerr,                                                                                     \
					PACEMAKER_RESET,                                                                               \
					std::forward<decltype(PACEMAKER_VAR(args))>(PACEMAKER_VAR(args))...)));                        \
			}                                                                                                      \
                                                                                                                   \
			PACEMAKER_DBG_RUN((pacemaker::print(std::cerr, '\n', PACEMAKER_RESET)));                               \
		}(__VA_ARGS__);                                                                                            \
	} while (0)
}  // namespace pacemaker

// Utilities
namespace pacemaker {
	template <typename T, typename... Ts> [[nodiscard]] constexpr bool any(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) or std::forward<Ts>(rest)) or ...);
	}

	template <typename T, typename... Ts> [[nodiscard]] constexpr bool all(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) and std::forward<Ts>(rest)) and ...);
	}

	template <typename T, typename... Ts> [[nodiscard]] constexpr bool none(T&& first, Ts&&... rest) {
		return ((not std::forward<T>(first) and not std::forward<Ts>(rest)) and ...);
	}

	template <typename T, typename... Ts> [[nodiscard]] constexpr bool cmp_all(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) == std::forward<Ts>(rest)) and ...);
	}

	template <typename T, typename... Ts> [[nodiscard]] constexpr bool cmp_any(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) == std::forward<Ts>(rest)) or ...);
	}

	template <typename T, typename... Ts> [[nodiscard]] constexpr bool cmp_none(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) != std::forward<Ts>(rest)) and ...);
	}
}  // namespace pacemaker

// Errors/Warnings
namespace pacemaker {
	template <typename... Ts> inline void info(Ts&&... args) {
		println(std::cerr, detail::log_to_str(LogLevel::INF), " ", std::forward<Ts>(args)..., PACEMAKER_RESET);
	}

	template <typename... Ts> inline void warning(Ts&&... args) {
		println(std::cerr, detail::log_to_str(LogLevel::WRN), " ", std::forward<Ts>(args)..., PACEMAKER_RESET);
	}

	template <typename... Ts> [[noreturn]] inline void fatal_error(Ts&&... args) {
		println(std::cerr, detail::log_to_str(LogLevel::ERR), " ", std::forward<Ts>(args)..., PACEMAKER_RESET);
		throw Fatal {};
	}

	template <typename... Ts> inline void error(Ts&&... args) {
		println(std::cerr, detail::log_to_str(LogLevel::ERR), " ", std::forward<Ts>(args)..., PACEMAKER_RESET);
	}

	template <typename... Ts> inline void ok(Ts&&... args) {
		println(std::cerr, detail::log_to_str(LogLevel::OK), " ", std::forward<Ts>(args)..., PACEMAKER_RESET);
	}
}  // namespace pacemaker

#endif