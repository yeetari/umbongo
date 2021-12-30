#pragma once

#include <kernel/SpinLock.hh>
#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <kernel/ipc/DoubleBuffer.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace kernel {

class Pipe final : public File {
    DoubleBuffer m_buffer;
    uint32 m_reader_count{0};
    uint32 m_writer_count{0};
    mutable SpinLock m_lock;

public:
    Pipe();
    Pipe(const Pipe &) = delete;
    Pipe(Pipe &&) = delete;
    ~Pipe() override = default;

    Pipe &operator=(const Pipe &) = delete;
    Pipe &operator=(Pipe &&) = delete;

    bool is_pipe() const override { return true; }

    void attach(AttachDirection) override;
    void detach(AttachDirection) override;
    bool read_would_block(usize offset) const override;
    bool write_would_block(usize offset) const override;
    SysResult<usize> read(ustd::Span<void> data, usize offset) override;
    SysResult<usize> write(ustd::Span<const void> data, usize offset) override;
};

} // namespace kernel
