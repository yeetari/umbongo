#include "StorageDevice.hh"

#include "Common.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Endpoint.hh"
#include "Error.hh"
#include "TrbRing.hh"

#include <core/EventLoop.hh>
#include <core/File.hh>
#include <core/Syscall.hh>
#include <kernel/api/UserFs.hh>
#include <log/Log.hh>
#include <ustd/Array.hh>
#include <ustd/Try.hh>

namespace {

struct [[gnu::packed]] CommandBlock {
    uint32 signature;
    uint32 tag;
    uint32 transfer_length;
    uint8 flags;
    uint8 lun;
    uint8 command_length;
    ustd::Array<uint8, 16> command_data;
};

struct [[gnu::packed]] CommandStatus {
    uint32 signature;
    uint32 tag;
    uint32 data_residue;
    uint8 status;
};

struct Read10 {
    uint8 opcode;
    uint8 options;
    ustd::Array<uint8, 4> lba;
    uint8 group_number;
    ustd::Array<uint8, 2> transfer_length;
    uint8 control;
};

} // namespace

ustd::Result<void, ustd::ErrorUnion<DeviceError, ScsiError>> StorageDevice::read_sector(ustd::Span<uint8> data,
                                                                                        uint32 lba) {
    CommandBlock command_block{
        .signature = 0x43425355,
        .tag = lba,
        .transfer_length = 512,
        .flags = 1u << 7u,
        .command_length = sizeof(Read10),
    };
    Read10 command{
        .opcode = 0x28,
        .lba{
            static_cast<uint8>((lba >> 24u) & 0xffu),
            static_cast<uint8>((lba >> 16u) & 0xffu),
            static_cast<uint8>((lba >> 8u) & 0xffu),
            static_cast<uint8>((lba >> 0u) & 0xffu),
        },
        .transfer_length{
            0,
            1,
        },
    };
    __builtin_memcpy(command_block.command_data.data(), &command, sizeof(Read10));

    TRY(m_output_endpoint->transfer_bulk({&command_block, sizeof(CommandBlock)}));
    TRY(m_input_endpoint->transfer_bulk(data));

    CommandStatus command_status{};
    TRY(m_input_endpoint->transfer_bulk({&command_status, sizeof(CommandStatus)}));
    if (command_status.signature != 0x53425355 || command_status.tag != lba) {
        return DeviceError::UnexpectedState;
    }
    if (command_status.status != 0u) {
        switch (command_status.status) {
        case 1:
            return ScsiError::CommandFailed;
        case 2:
            return ScsiError::PhaseError;
        default:
            return ScsiError::Unknown;
        }
    }
    if (command_status.data_residue != 0u) {
        return ScsiError::ShortRead;
    }
    return {};
}

ustd::Result<void, DeviceError> StorageDevice::enable(core::EventLoop &event_loop) {
    TRY(walk_configuration([this](void *descriptor, DescriptorType type) -> ustd::Result<void, DeviceError> {
        if (type == DescriptorType::Configuration) {
            auto *config_descriptor = static_cast<ConfigDescriptor *>(descriptor);
            TRY(set_configuration(config_descriptor->config_value));
        } else if (type == DescriptorType::Endpoint) {
            auto *endpoint_descriptor = static_cast<EndpointDescriptor *>(descriptor);
            if ((endpoint_descriptor->address & (1u << 7u)) == 0u && m_output_endpoint == nullptr) {
                m_output_endpoint = &create_endpoint(endpoint_descriptor->address);
                EXPECT(m_output_endpoint->setup(EndpointType::BulkOut, endpoint_descriptor->packet_size));
            } else if (m_input_endpoint == nullptr) {
                ASSERT(m_input_endpoint == nullptr);
                m_input_endpoint = &create_endpoint(endpoint_descriptor->address);
                EXPECT(m_input_endpoint->setup(EndpointType::BulkIn, endpoint_descriptor->packet_size));
            }
        }
        return {};
    }));

    m_file = core::File(EXPECT(core::syscall<uint32>(Syscall::create_user_fs, "/dev/sda")));
    m_file.set_on_read_ready([this] {
        auto request = EXPECT(m_file.read<kernel::UserFsRequest>());
        if (request.data_size != 0) {
            auto *data = new uint8[request.data_size];
            EXPECT(m_file.read({data, request.data_size}));
        }
        switch (request.opcode) {
        case kernel::UserFsOpcode::Read: {
            // NOLINTNEXTLINE
            ustd::Array<uint8, 512> sector;
            EXPECT(read_sector(sector.span(), static_cast<uint32>(request.offset / 512)));
            struct {
                kernel::UserFsResponse header;
                ustd::Array<uint8, 512> data;
            } response{
                .header{
                    .data_size = 512 - (request.offset % 512),
                },
            };
            __builtin_memcpy(response.data.data(), sector.data() + (request.offset % 512), response.header.data_size);
            EXPECT(m_file.write({&response, sizeof(kernel::UserFsResponse) + response.header.data_size}));
            break;
        }
        default:
            EXPECT(m_file.write(kernel::UserFsResponse{
                .error = core::SysError::Invalid,
            }));
            break;
        }
    });
    event_loop.watch(m_file, kernel::PollEvents::Read);
    return {};
}
