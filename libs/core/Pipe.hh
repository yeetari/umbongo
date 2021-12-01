#pragma once

#include <core/File.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace core {

class Pipe {
    File m_read_end;
    File m_write_end;

public:
    Pipe(File &&read_end, File &&write_end) : m_read_end(ustd::move(read_end)), m_write_end(ustd::move(write_end)) {}

    ssize rebind_read(uint32 fd);
    ssize rebind_write(uint32 fd);
};

ustd::Optional<Pipe> create_pipe();

} // namespace core
