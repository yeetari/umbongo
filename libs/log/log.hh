#pragma once

#include <log/level.hh>
#include <ustd/string_builder.hh>
#include <ustd/string_view.hh>
#include <ustd/utility.hh>

namespace log {

void initialise(ustd::StringView name);
void message(Level level, ustd::StringView message);

template <typename... Args>
void trace(const char *fmt, Args &&...args) {
    message(Level::Trace, ustd::format(fmt, ustd::forward<Args>(args)...));
}

template <typename... Args>
void debug(const char *fmt, Args &&...args) {
    message(Level::Debug, ustd::format(fmt, ustd::forward<Args>(args)...));
}

template <typename... Args>
void info(const char *fmt, Args &&...args) {
    message(Level::Info, ustd::format(fmt, ustd::forward<Args>(args)...));
}

template <typename... Args>
void warn(const char *fmt, Args &&...args) {
    message(Level::Warn, ustd::format(fmt, ustd::forward<Args>(args)...));
}

template <typename... Args>
void error(const char *fmt, Args &&...args) {
    message(Level::Error, ustd::format(fmt, ustd::forward<Args>(args)...));
}

} // namespace log
