#pragma once


#include <string_view>


namespace tslog {


enum class Level { Trace, Debug, Info, Warn, Error, Fatal };


constexpr std::string_view to_string(Level lvl) noexcept {
switch (lvl) {
case Level::Trace: return "TRACE";
case Level::Debug: return "DEBUG";
case Level::Info: return "INFO";
case Level::Warn: return "WARN";
case Level::Error: return "ERROR";
case Level::Fatal: return "FATAL";
}
return "INFO";
}


} // namespace tslog