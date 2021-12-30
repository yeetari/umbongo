#pragma once

#include <ipc/Message.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/MessageEncoder.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace config {

enum class MessageKind {
    NotifyChange,
    ListAll,
    ListAllResponse,
    Lookup,
    LookupResponse,
    Read,
    Update,
    UpdateResponse,
    Watch,
};

class NotifyChangeMessage final : public ipc::Message {
    ustd::StringView m_key;
    ustd::StringView m_value;

public:
    NotifyChangeMessage(ustd::StringView key, ustd::StringView value) : m_key(key), m_value(value) {}

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::NotifyChange);
        encoder.encode(m_key);
        encoder.encode(m_value);
    }
};

class ListAllMessage final : public ipc::Message {
public:
    void encode(ipc::MessageEncoder &encoder) const override { encoder.encode(MessageKind::ListAll); }
};

class ListAllResponseMessage final : public ipc::Message {
    ustd::Vector<ustd::String> m_list;

public:
    ListAllResponseMessage(ustd::Vector<ustd::String> &&list) : m_list(ustd::move(list)) {}

    static ListAllResponseMessage decode(ustd::Span<const uint8> buffer) {
        ipc::MessageDecoder decoder(buffer);
        [[maybe_unused]] auto kind = decoder.decode<MessageKind>();
        ASSERT(kind == MessageKind::ListAllResponse);
        ustd::Vector<ustd::String> list;
        list.ensure_capacity(decoder.decode<uint32>());
        for (uint32 i = 0; i < list.capacity(); i++) {
            list.emplace(decoder.decode<ustd::StringView>());
        }
        return ustd::move(list);
    }

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::ListAllResponse);
        encoder.encode(m_list.size());
        for (const auto &elem : m_list) {
            encoder.encode(elem);
        }
    }

    ustd::Vector<ustd::String> &list() { return m_list; }
};

class LookupMessage final : public ipc::Message {
    ustd::StringView m_domain;
    ustd::StringView m_key;

public:
    LookupMessage(ustd::StringView domain, ustd::StringView key) : m_domain(domain), m_key(key) {}

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::Lookup);
        encoder.encode(m_domain);
        encoder.encode(m_key);
    }
};

class LookupResponseMessage final : public ipc::Message {
    ustd::String m_value;
    bool m_present;

public:
    LookupResponseMessage(ustd::StringView value, bool present) : m_value(value), m_present(present) {}

    static LookupResponseMessage decode(ustd::Span<const uint8> buffer) {
        ipc::MessageDecoder decoder(buffer);
        [[maybe_unused]] auto kind = decoder.decode<MessageKind>();
        ASSERT(kind == MessageKind::LookupResponse);
        return {decoder.decode<ustd::StringView>(), decoder.decode<bool>()};
    }

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::LookupResponse);
        encoder.encode(m_value);
        encoder.encode(m_present);
    }

    ustd::Optional<ustd::String> value() const {
        return m_present ? ustd::move(m_value) : ustd::Optional<ustd::String>();
    }
};

class ReadMessage final : public ipc::Message {
    ustd::StringView m_domain;

public:
    explicit ReadMessage(ustd::StringView domain) : m_domain(domain) {}

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::Read);
        encoder.encode(m_domain);
    }
};

class UpdateMessage final : public ipc::Message {
    ustd::StringView m_domain;
    ustd::StringView m_key;
    ustd::StringView m_value;

public:
    UpdateMessage(ustd::StringView domain, ustd::StringView key, ustd::StringView value)
        : m_domain(domain), m_key(key), m_value(value) {}

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::Update);
        encoder.encode(m_domain);
        encoder.encode(m_key);
        encoder.encode(m_value);
    }
};

class UpdateResponseMessage final : public ipc::Message {
    bool m_success;

public:
    explicit UpdateResponseMessage(bool success) : m_success(success) {}

    static UpdateResponseMessage decode(ustd::Span<const uint8> buffer) {
        ipc::MessageDecoder decoder(buffer);
        [[maybe_unused]] auto kind = decoder.decode<MessageKind>();
        ASSERT(kind == MessageKind::UpdateResponse);
        return UpdateResponseMessage(decoder.decode<bool>());
    }

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::UpdateResponse);
        encoder.encode(m_success);
    }

    bool success() const { return m_success; }
};

class WatchMessage final : public ipc::Message {
    ustd::StringView m_domain;

public:
    explicit WatchMessage(ustd::StringView domain) : m_domain(domain) {}

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::Watch);
        encoder.encode(m_domain);
    }
};

} // namespace config
