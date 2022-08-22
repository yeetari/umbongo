#include <core/UserFs.hh>

#include <core/Syscall.hh>
#include <kernel/api/UserFs.hh>
#include <ustd/Try.hh>

namespace core {

ustd::Result<void, SysError> UserFs::bind(ustd::StringView path) {
    m_file = core::File(TRY(core::syscall<uint32>(Syscall::create_user_fs, path.data())));
    m_file.set_on_read_ready([this] {
        EXPECT(m_file.read<kernel::UserFsRequest>());
    });
}

} // namespace core
