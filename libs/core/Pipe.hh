#pragma once

#include <core/File.hh>
#include <kernel/SysError.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class Pipe {
    File m_read_end;
    File m_write_end;

public:
    Pipe() = default;
    Pipe(File &&read_end, File &&write_end) : m_read_end(ustd::move(read_end)), m_write_end(ustd::move(write_end)) {}

    ustd::Result<void, SysError> rebind_read(uint32 fd);
    ustd::Result<void, SysError> rebind_write(uint32 fd);
    uint32 read_fd() const { return m_read_end.fd(); }
    uint32 write_fd() const { return m_write_end.fd(); }
};

ustd::Result<Pipe, SysError> create_pipe();

} // namespace core
