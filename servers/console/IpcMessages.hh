#pragma once

#include <ipc/Message.hh>
#include <ipc/MessageDecoder.hh>
#include <ipc/MessageEncoder.hh>
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
    uint32 m_column_count;
    uint32 m_row_count;

public:
    GetTerminalSizeRespone(uint32 column_count, uint32 row_count)
        : m_column_count(column_count), m_row_count(row_count) {}

    static GetTerminalSizeRespone decode(Span<const uint8> buffer) {
        ipc::MessageDecoder decoder(buffer);
        ASSERT(decoder.decode<MessageKind>() == MessageKind::GetTerminalSizeResponse);
        return {decoder.decode<uint32>(), decoder.decode<uint32>()};
    }

    void encode(ipc::MessageEncoder &encoder) const override {
        encoder.encode(MessageKind::GetTerminalSizeResponse);
        encoder.encode(m_column_count);
        encoder.encode(m_row_count);
    }

    uint32 column_count() const { return m_column_count; }
    uint32 row_count() const { return m_row_count; }
};

} // namespace console
