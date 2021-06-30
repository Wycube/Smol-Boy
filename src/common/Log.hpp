#ifndef LOG_HPP
#define LOG_HPP

#if defined(__GNU__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif


#include <string_view>
#include <fmt/format.h>

namespace logger {

enum Level {
    Info, Warning, Error, Debug, Fatal
};

void log_debug(Level level, const char *file, int line, const char *func, const std::string_view &message);
void log(Level level, const std::string_view &message);

#define LOG_INFO(message, ...)  logger::log(logger::Info, fmt::format(message, ## __VA_ARGS__))
#define LOGD_INFO(message, ...)  logger::log_debug(logger::Info, __FILE__, __LINE__, __func__, fmt::format(message, ## __VA_ARGS__))
#define LOG_WARN(message, ...)  logger::log(logger::Warning, fmt::format(message, ## __VA_ARGS__))
#define LOGD_WARN(message, ...)  logger::log_debug(logger::Warning, __FILE__, __LINE__, __func__, fmt::format(message, ## __VA_ARGS__))
#define LOG_ERROR(message, ...) logger::log(logger::Error, fmt::format(message, ## __VA_ARGS__))
#define LOGD_ERROR(message, ...) logger::log_debug(logger::Error, __FILE__, __LINE__, __func__, fmt::format(message, ## __VA_ARGS__))
#define LOG_FATAL(message, ...) logger::log(logger::Fatal, fmt::format(message, ## __VA_ARGS__)); ::exit(-1)
#define LOGD_FATAL(message, ...) logger::log_debug(logger::Fatal, __FILE__, __LINE__, __func__, fmt::format(message, ## __VA_ARGS__)); ::exit(-1)

#ifdef _DEBUG
    #define LOG_DEBUG(message, ...) logger::log(logger::Debug, fmt::format(message, ## __VA_ARGS__))
    #define LOGD_DEBUG(message, ...) logger::log_debug(logger::Debug, __FILE__, __LINE__, __func__, fmt::format(message, ## __VA_ARGS__))
#else
    #define LOG_DEBUG(message, ...)
    #define LOGD_DEBUG(message, ...)
#endif

} //namespace logger


#if defined(__GNU__)
    #pragma GCC diagnostic pop
#elif defined(__clang__)
    #pragma clang diagnostic pop
#endif

#endif //LOG_HPP