#pragma once

#include <kernel/spin_lock.hh>
#include <ustd/shareable.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/types.hh>

namespace kernel {

class DoubleBuffer : public ustd::Shareable<DoubleBuffer> {
    struct Buffer {
        uint8_t *data{nullptr};
        size_t size{0};
    };
    uint8_t *m_data;
    size_t m_size;
    Buffer m_buffer1;
    Buffer m_buffer2;
    Buffer *m_read_buffer;
    Buffer *m_write_buffer;
    size_t m_read_position{0};
    mutable SpinLock m_lock;

public:
    explicit DoubleBuffer(size_t size);
    DoubleBuffer(const DoubleBuffer &) = delete;
    DoubleBuffer(DoubleBuffer &&) = delete;
    ~DoubleBuffer();

    DoubleBuffer &operator=(const DoubleBuffer &) = delete;
    DoubleBuffer &operator=(DoubleBuffer &&) = delete;

    bool empty() const;
    bool full() const;
    size_t read(ustd::Span<void> data);
    size_t write(ustd::Span<const void> data);
};

} // namespace kernel
