#pragma once

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

    usize read(Span<void>, usize) override { return 0; }
    usize size() override { return 0; }
    usize write(Span<const void>, usize) override { return 0; }

    void disconnect();
    bool valid() override { return m_connected; }

    virtual const char *name() const { ENSURE_NOT_REACHED(); }
};
