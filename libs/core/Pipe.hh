#pragma once

#include <core/File.hh>
#include <system/Error.h>
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

    void close_read() { m_read_end.close(); }
    void close_write() { m_write_end.close(); }
    ustd::Result<void, ub_error_t> rebind_read(uint32_t fd);
    ustd::Result<void, ub_error_t> rebind_write(uint32_t fd);
    uint32_t read_fd() const { return m_read_end.fd(); }
    uint32_t write_fd() const { return m_write_end.fd(); }
};

ustd::Result<Pipe, ub_error_t> create_pipe();

} // namespace core
