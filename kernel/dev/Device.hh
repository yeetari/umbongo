#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/Span.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace kernel {

class Device : public File {
    const ustd::String m_path;
    bool m_connected{true};

public:
    explicit Device(ustd::String &&path);

    SysResult<usize> read(ustd::Span<void>, usize) override { return 0u; }
    SysResult<usize> write(ustd::Span<const void>, usize) override { return 0u; }

    void disconnect();
    bool valid() override { return m_connected; }
    ustd::StringView path() const { return m_path; }
};

} // namespace kernel
