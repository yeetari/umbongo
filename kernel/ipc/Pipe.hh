#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/fs/File.hh>
#include <kernel/ipc/DoubleBuffer.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

class Pipe final : public File {
    DoubleBuffer m_buffer;
    uint32 m_reader_count{0};
    uint32 m_writer_count{0};
    SpinLock m_lock;

public:
    Pipe();
    Pipe(const Pipe &) = delete;
    Pipe(Pipe &&) = delete;
    ~Pipe() override = default;

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
