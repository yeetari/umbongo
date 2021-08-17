#pragma once

#include <kernel/fs/File.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

class Pipe final : public File {
    struct Buffer {
        uint8 *data{nullptr};
        usize size{0};
    };
    uint8 *m_data;
    Buffer m_buffer1;
    Buffer m_buffer2;
    Buffer *m_read_buffer;
    Buffer *m_write_buffer;
    usize m_read_position{0};
    uint32 m_reader_count{0};
    uint32 m_writer_count{0};

public:
    Pipe();
    Pipe(const Pipe &) = delete;
    Pipe(Pipe &&) = delete;
    ~Pipe() override;

    Pipe &operator=(const Pipe &) = delete;
    Pipe &operator=(Pipe &&) = delete;

    void attach(AttachDirection) override;
    void detach(AttachDirection) override;
    bool can_read() override;
    bool can_write() override;
    usize read(Span<void> data, usize offset) override;
    usize size() override;
    usize write(Span<const void> data, usize offset) override;
};
