#pragma once

#include <kernel/fs/file.hh>
#include <kernel/ipc/double_buffer.hh>
#include <kernel/spin_lock.hh>
#include <kernel/sys_result.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/types.hh>

namespace kernel {

class Pipe final : public File {
    DoubleBuffer m_buffer;
    uint32_t m_reader_count{0};
    uint32_t m_writer_count{0};
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
    bool read_would_block(size_t offset) const override;
    bool write_would_block(size_t offset) const override;
    SysResult<size_t> read(ustd::Span<void> data, size_t offset) override;
    SysResult<size_t> write(ustd::Span<const void> data, size_t offset) override;
};

} // namespace kernel
