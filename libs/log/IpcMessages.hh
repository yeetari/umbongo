#pragma once

#include <ipc/Decoder.hh>
#include <ipc/Encoder.hh>
#include <log/Level.hh>

namespace log {

enum class MessageKind {
    Initialise,
    Log,
};

class InitialiseMessage final : public ipc::Message {
    ustd::StringView m_name;

public:
    explicit InitialiseMessage(ustd::StringView name) : m_name(name) {}

    void encode(ipc::Encoder &encoder) const override {
        encoder.encode(MessageKind::Initialise);
        encoder.encode(m_name);
    }
};

class LogMessage final : public ipc::Message {
    ustd::StringView m_message;
    Level m_level;

public:
    LogMessage(Level level, ustd::StringView message) : m_message(message), m_level(level) {}

    void encode(ipc::Encoder &encoder) const override {
        encoder.encode(MessageKind::Log);
        encoder.encode(m_level);
        encoder.encode(m_message);
    }
};

} // namespace log
