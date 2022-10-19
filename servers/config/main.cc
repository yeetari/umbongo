#include <config/IpcMessages.hh>
#include <core/EventLoop.hh>
#include <core/File.hh>
#include <ipc/Client.hh>
#include <ipc/Decoder.hh>
#include <ipc/Server.hh>
#include <log/Log.hh>
#include <ustd/Format.hh>
#include <ustd/Optional.hh>
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

class Domain {
    ustd::String m_name;
    core::File m_config_file;
    struct KeyValue {
        ustd::String key;
        ustd::String value;
    };
    ustd::Vector<KeyValue> m_key_values;
    ustd::Vector<ipc::Client *> m_watchers;

public:
    explicit Domain(ustd::StringView name);

    ustd::Optional<ustd::StringView> lookup(ustd::StringView key) const;
    bool update(ustd::StringView key, ustd::StringView value);
    void watch(ipc::Client &client);

    ustd::StringView name() const { return m_name; }
    const ustd::Vector<KeyValue> &key_values() const { return m_key_values; }
};

Domain::Domain(ustd::StringView name) : m_name(name) {
    // TODO: Allow failure.
    auto config_path = ustd::format("/etc/{}.cfg", name);
    log::debug("Reading config for {} ({})", name, config_path);
    m_config_file = EXPECT(core::File::open(config_path));

    ustd::LargeVector<char> config_source(EXPECT(m_config_file.size()));
    EXPECT(m_config_file.read(config_source.span()));

    ustd::Vector<ustd::StringView> lines;
    for (auto *line = &lines.emplace(config_source.data(), 0u); const auto &ch : config_source) {
        if (ch == '\n') {
            line = &lines.emplace(&ch + 1, 0u);
            continue;
        }
        *line = {line->data(), line->length() + 1u};
    }

    for (auto line : lines) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        size_t equal_index = 0;
        for (auto ch : line) {
            if (ch == '=') {
                break;
            }
            equal_index++;
        }
        ustd::String key(line.substr(0, equal_index));
        ustd::String value(line.substr(equal_index + 1));
        m_key_values.emplace(KeyValue{ustd::move(key), ustd::move(value)});
    }
}

ustd::Optional<ustd::StringView> Domain::lookup(ustd::StringView key) const {
    for (const auto &pair : m_key_values) {
        if (pair.key == key) {
            // TODO: template <typename U> Optional constructor.
            return static_cast<ustd::StringView>(pair.value);
        }
    }
    return {};
}

bool Domain::update(ustd::StringView key, ustd::StringView value) {
    for (auto &pair : m_key_values) {
        if (pair.key == key) {
            ustd::String new_value(value);
            pair.value = ustd::move(new_value);
            for (auto *watcher : m_watchers) {
                watcher->send_message<config::NotifyChangeMessage>(key, value);
            }
            return true;
        }
    }
    return false;
}

void Domain::watch(ipc::Client &client) {
    for (auto *watcher : m_watchers) {
        if (watcher == &client) {
            return;
        }
    }
    m_watchers.push(&client);
}

} // namespace

size_t main(size_t, const char **) {
    log::initialise("config-server");
    ustd::Vector<Domain> domains;
    core::EventLoop event_loop;
    ipc::Server<ipc::Client> server(event_loop, "/run/config"sv);
    server.set_on_message([&](ipc::Client &client, ipc::MessageDecoder &decoder) {
        using namespace config;
        switch (auto message_kind = decoder.decode<MessageKind>()) {
        case MessageKind::Read: {
            // TODO: Handle domain already existing.
            auto domain_name = decoder.decode<ustd::StringView>();
            domains.emplace(domain_name);
            return true;
        }
        case MessageKind::ListAll: {
            ustd::Vector<ustd::String> list;
            for (const auto &domain : domains) {
                for (const auto &pair : domain.key_values()) {
                    list.push(ustd::format("{}.{}={}", domain.name(), pair.key, pair.value));
                }
            }
            client.send_message<ListAllResponseMessage>(ustd::move(list));
            return true;
        }
        case MessageKind::Lookup: {
            auto domain_name = decoder.decode<ustd::StringView>();
            auto key = decoder.decode<ustd::StringView>();
            for (const auto &domain : domains) {
                if (domain.name() != domain_name) {
                    continue;
                }
                if (auto value = domain.lookup(key)) {
                    client.send_message<LookupResponseMessage>(*value, true);
                    return true;
                }
            }
            client.send_message<LookupResponseMessage>(ustd::StringView(), false);
            return true;
        }
        case MessageKind::Update: {
            auto domain_name = decoder.decode<ustd::StringView>();
            auto key = decoder.decode<ustd::StringView>();
            auto value = decoder.decode<ustd::StringView>();
            for (auto &domain : domains) {
                if (domain.name() == domain_name && domain.update(key, value)) {
                    client.send_message<UpdateResponseMessage>(true);
                    return true;
                }
            }
            client.send_message<UpdateResponseMessage>(false);
            return true;
        }
        case MessageKind::Watch: {
            auto domain_name = decoder.decode<ustd::StringView>();
            for (auto &domain : domains) {
                if (domain.name() == domain_name) {
                    domain.watch(client);
                    for (const auto &pair : domain.key_values()) {
                        client.send_message<NotifyChangeMessage>(pair.key, pair.value);
                    }
                }
            }
            return true;
        }
        default:
            log::warn("Received unknown message {}", static_cast<size_t>(message_kind));
            return false;
        }
    });
    return event_loop.run();
}
