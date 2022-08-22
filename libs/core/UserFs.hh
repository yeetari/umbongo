#pragma once

#include <core/Error.hh>
#include <core/File.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>

namespace core {

class UserFs {
    File m_file;

public:
    UserFs() = default;
    UserFs(const UserFs &) = delete;
    UserFs(UserFs &&) = delete;
    virtual ~UserFs() = default;

    UserFs &operator=(const UserFs &) = delete;
    UserFs &operator=(UserFs &&) = delete;

    ustd::Result<void, SysError> bind(ustd::StringView path);
    virtual ustd::Result<usize, SysError> read(usize offset) = 0;
};

} // namespace core
