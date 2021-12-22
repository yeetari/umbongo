#include "KeyboardDevice.hh"

#include "Common.hh"
#include "Descriptor.hh"
#include "Endpoint.hh"
#include "Error.hh"
#include "TrbRing.hh"

#include <core/Pipe.hh>
#include <kernel/KeyEvent.hh>
#include <kernel/Syscall.hh>
#include <mmio/Mmio.hh>
#include <ustd/Array.hh>
#include <ustd/Memory.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace {

constexpr ustd::Array s_scancode_table{
    '\0', '\0', '\0', '\0', 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',
    'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2',  '3',  '4',
    '5',  '6',  '7',  '8',  '9',  '0',  '\n', '\0', '\b', '\t', ' ',  '-',  '=',  '[',  ']',  '#',  '\0',
    ';',  '\'', '`',  ',',  '.',  '/',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\\',
};

constexpr ustd::Array s_scancode_table_shift{
    '\0', '\0', '\0', '\0', 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',
    'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '"',  '\0', '$',
    '%',  '^',  '&',  '*',  '(',  ')',  '\n', '\0', '\b', '\0', ' ',  '_',  '+',  '{',  '}',  '~',  '\0',
    ':',  '@',  '\0', '<',  '>',  '?',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '|',
};

} // namespace

ustd::Result<void, DeviceError> KeyboardDevice::enable() {
    m_dma_buffer = EXPECT(mmio::alloc_dma_array<uint8>(8));
    m_pipe = EXPECT(core::create_pipe());
    EXPECT(Syscall::invoke(Syscall::bind, m_pipe.read_fd(), "/dev/kb"));
    TRY(walk_configuration([this](void *descriptor, DescriptorType type) -> ustd::Result<void, DeviceError> {
        if (type == DescriptorType::Configuration) {
            auto *config_descriptor = static_cast<ConfigDescriptor *>(descriptor);
            TRY(set_configuration(config_descriptor->config_value));
        } else if (type == DescriptorType::Endpoint) {
            auto *endpoint_descriptor = static_cast<EndpointDescriptor *>(descriptor);
            if ((endpoint_descriptor->address & (1u << 7u)) == 0u) {
                return {};
            }
            // TODO: Allow failure.
            auto &input_endpoint = create_endpoint(endpoint_descriptor->address);
            EXPECT(input_endpoint.setup(EndpointType::InterruptIn, endpoint_descriptor->packet_size));
            input_endpoint.set_interval(endpoint_descriptor->interval);
            for (usize i = 0; i < 255; i++) {
                input_endpoint.transfer_ring().enqueue({
                    .data = EXPECT(mmio::virt_to_phys(m_dma_buffer)),
                    .status = 8u,
                    .event_on_completion = true,
                    .type = TrbType::Normal,
                });
            }
        }
        return {};
    }));
    return {};
}

bool KeyboardDevice::key_already_pressed(uint8 key) const {
    for (uint8 i = 2; i < 8; i++) {
        if (key == m_cmp_buffer[i]) {
            return true;
        }
    }
    return false;
}

void KeyboardDevice::poll() {
    for (uint8 i = 0; i < 8; i++) {
        bool pressed_now = (m_dma_buffer[0] & (1u << i)) != 0;
        bool pressed_bef = (m_cmp_buffer[0] & (1u << i)) != 0;
        m_modifiers[i] = (pressed_now && !pressed_bef) || pressed_bef;
    }
    for (uint8 i = 2; i < 8; i++) {
        const uint8 key = m_dma_buffer[i];
        if (key_already_pressed(key)) {
            continue;
        }
        const auto &table = m_modifiers[1] ? s_scancode_table_shift : s_scancode_table;
        KeyEvent key_event{
            key,
            key < table.size() ? table[key] : '\0',
            m_modifiers[2] || m_modifiers[6],
            m_modifiers[0] || m_modifiers[4],
        };
        EXPECT(Syscall::invoke(Syscall::write, m_pipe.write_fd(), &key_event, sizeof(KeyEvent)));
    }
    __builtin_memcpy(m_cmp_buffer.data(), m_dma_buffer, 8);
}