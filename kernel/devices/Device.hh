#pragma once

#include <kernel/SysResult.hh>
#include <kernel/fs/File.hh>
#include <ustd/Assert.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

class Device : public File {
    bool m_connected{true};

public:
    static Vector<Device *> all_devices();

    Device();
    Device(const Device &) = delete;
    Device(Device &&) = delete;
    ~Device() override;

    Device &operator=(const Device &) = delete;
    Device &operator=(Device &&) = delete;

    SysResult<usize> read(Span<void>, usize) override { return 0u; }
    SysResult<usize> write(Span<const void>, usize) override { return 0u; }

    void disconnect();
    bool valid() override { return m_connected; }

    virtual const char *name() const { ENSURE_NOT_REACHED(); }
};
