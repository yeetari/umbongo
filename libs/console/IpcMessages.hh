#pragma once

#include <ipc/Decoder.hh>
#include <ipc/Encoder.hh>
#include <ipc/Message.hh>
#include <ustd/Assert.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace console {

enum class MessageKind {
    GetTerminalSize,
    GetTerminalSizeResponse,
};

class GetTerminalSize final : public ipc::Message {
public:
    void encode(ipc::MessageEncoder &encoder) const override { encoder.encode(MessageKind::GetTerminalSize); }
};

class GetTerminalSizeRespone final : public ipc::Message {
    uint32_t m_column_count;
    uint32_t m_row_count;

public:
    GetTerminalSizeRespone(uint32_t column_count, uint32_t row_count)
        : m_column_count(column_count), m_row_count(row_count) {}

    static GetTerminalSizeRespone decode(ustd::Span<const uint8_t> buffer) {
        ipc::MessageDecoder decoder(buffer);
        [[maybe_unused]] auto kind = decoder.decode<MessageKind>();
        ASSERT(kind == MessageKind::GetTerminalSizeResponse);
        return {decoder.decode<uint32_t>(), decoder.decode<uint32_t>()};
    }

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::GetTerminalSizeResponse);
        encoder.encode(m_column_count);
        encoder.encode(m_row_count);
    }

    uint32_t column_count() const { return m_column_count; }
    uint32_t row_count() const { return m_row_count; }
};

} // namespace console
