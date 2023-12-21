#pragma once

#include <kernel/fs/file.hh>
#include <kernel/sys_result.hh>
#include <ustd/span.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>

namespace kernel {

class Device : public File {
    const ustd::String m_path;
    bool m_connected{true};

public:
    explicit Device(ustd::String &&path);

    SysResult<size_t> read(ustd::Span<void>, size_t) override { return 0u; }
    SysResult<size_t> write(ustd::Span<const void>, size_t) override { return 0u; }

    void disconnect();
    bool valid() const override { return m_connected; }
    ustd::StringView path() const { return m_path; }
};

} // namespace kernel
