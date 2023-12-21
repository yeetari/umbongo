#pragma once

#include <ustd/function.hh> // IWYU pragma: keep
#include <ustd/optional.hh>
#include <ustd/string.hh> // IWYU pragma: keep
#include <ustd/string_view.hh>
#include <ustd/vector.hh>
// IWYU pragma: no_forward_declare ustd::Function

namespace core {

class EventLoop;

} // namespace core

namespace config {

void listen(core::EventLoop &event_loop);
void read(ustd::StringView domain);
ustd::Vector<ustd::String> list_all();
ustd::Optional<ustd::String> lookup(ustd::StringView domain, ustd::StringView key);
bool update(ustd::StringView domain, ustd::StringView key, ustd::StringView value);
void watch(ustd::StringView domain, ustd::StringView key, ustd::Function<void(ustd::StringView value)> callback);

} // namespace config
