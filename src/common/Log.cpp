#include "Log.hpp"
#include "Utility.hpp"

namespace logger {

void log_debug(Level level, const char *file, int line, const char *func, const std::string_view &message) {
    std::string prefix = "\33[";

    //Select Color
    switch(level) {
        case Level::Info :    prefix += "32m[I]"; break;
        case Level::Warning : prefix += "33m[W]"; break;
        case Level::Error :   prefix += "91m[E]"; break;
        case Level::Debug :   prefix += "34m[D]"; break;
        case Level::Fatal :   prefix += "31m[F]"; break;
    }

    fmt::print("{} {}:{}:{} {}\33[39m\n", prefix, base_name(file), line, func, message);
}

void log(Level level, const std::string_view &message) {
    std::string prefix = "\33[";

    //Select Color
    switch(level) {
        case Level::Info :    prefix += "32m[I]"; break;
        case Level::Warning : prefix += "33m[W]"; break;
        case Level::Error :   prefix += "91m[E]"; break;
        case Level::Debug :   prefix += "34m[D]"; break;
        case Level::Fatal :   prefix += "31m[F]"; break;
    }

    fmt::print("{} {}\33[39m\n", prefix, message);
}

} //namespace logger