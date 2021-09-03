#pragma once

namespace ipc {

class MessageEncoder;

class Message {
protected:
    Message() = default;

public:
    Message(const Message &) = delete;
    Message(Message &&) = delete;
    virtual ~Message() = default;

    Message &operator=(const Message &) = delete;
    Message &operator=(Message &&) = delete;

    virtual void encode(MessageEncoder &encoder) const = 0;
};

} // namespace ipc
