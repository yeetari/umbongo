#pragma once

#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/Optional.hh>
#include <ustd/String.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Vector.hh>

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
