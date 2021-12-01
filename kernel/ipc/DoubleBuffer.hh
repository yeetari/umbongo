#pragma once

#include <kernel/SpinLock.hh>
#include <ustd/Shareable.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

class DoubleBuffer : public ustd::Shareable<DoubleBuffer> {
    struct Buffer {
        uint8 *data{nullptr};
        usize size{0};
    };
    uint8 *m_data;
    usize m_size;
    Buffer m_buffer1;
    Buffer m_buffer2;
    Buffer *m_read_buffer;
    Buffer *m_write_buffer;
    usize m_read_position{0};
    mutable SpinLock m_lock;

public:
    explicit DoubleBuffer(usize size);
    DoubleBuffer(const DoubleBuffer &) = delete;
    DoubleBuffer(DoubleBuffer &&) = delete;
    ~DoubleBuffer();

    DoubleBuffer &operator=(const DoubleBuffer &) = delete;
    DoubleBuffer &operator=(DoubleBuffer &&) = delete;

    bool empty() const;
    bool full() const;
    usize read(ustd::Span<void> data);
    usize write(ustd::Span<const void> data);
};
