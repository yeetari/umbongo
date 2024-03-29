#include "keyboard_device.hh"

#include "common.hh"
#include "descriptor.hh"
#include "device.hh"
#include "endpoint.hh"
#include "trb_ring.hh"

#include <config/config.hh>
#include <core/key_event.hh>
#include <core/pipe.hh>
#include <core/time.hh>
#include <core/timer.hh>
#include <mmio/mmio.hh>
#include <system/syscall.hh>
#include <ustd/array.hh>
#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/result.hh>
#include <ustd/string_cast.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>

enum class DeviceError;

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

KeyboardDevice::KeyboardDevice(Device &&device) : Device(ustd::move(device)) {}
KeyboardDevice::~KeyboardDevice() = default;

ustd::Result<void, DeviceError> KeyboardDevice::enable(core::EventLoop &event_loop) {
    m_dma_buffer = EXPECT(mmio::alloc_dma_array<uint8_t>(8));
    m_pipe = EXPECT(core::create_pipe());
    EXPECT(system::syscall(UB_SYS_bind, m_pipe.read_fd(), "/dev/kb"));
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
            for (size_t i = 0; i < 255; i++) {
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

    // TODO: Disable timer if no key pressed?
    m_repeat_timer = ustd::make_unique<core::Timer>(event_loop, 25_Hz);
    m_repeat_timer->set_on_fire([this] {
        if (m_last_code == 0u || !key_already_pressed(m_last_code)) {
            return;
        }
        if (core::time() - m_last_time < m_repeat_delay * 1000000u) {
            return;
        }
        const auto &table = m_modifiers[1] ? s_scancode_table_shift : s_scancode_table;
        core::KeyEvent key_event{
            m_last_code,
            m_last_code < table.size() ? table[m_last_code] : '\0',
            m_modifiers[2] || m_modifiers[6],
            m_modifiers[0] || m_modifiers[4],
        };
        EXPECT(system::syscall(UB_SYS_write, m_pipe.write_fd(), &key_event, sizeof(core::KeyEvent)));
    });

    config::watch("usb-server", "keyboard.repeat_delay", [this](ustd::StringView value) {
        m_repeat_delay = ustd::cast<size_t>(value).value_or(400);
    });
    config::watch("usb-server", "keyboard.repeat_rate", [this](ustd::StringView value) {
        const auto repeat_rate = ustd::max(ustd::cast<size_t>(value).value_or(25), 1ul);
        m_repeat_timer->set_period(1_Hz / repeat_rate);
    });
    return {};
}

bool KeyboardDevice::key_already_pressed(uint8_t key) const {
    for (uint8_t i = 2; i < 8; i++) {
        if (key == m_cmp_buffer[i]) {
            return true;
        }
    }
    return false;
}

void KeyboardDevice::poll() {
    for (uint8_t i = 0; i < 8; i++) {
        bool pressed_now = (m_dma_buffer[0] & (1u << i)) != 0;
        bool pressed_bef = (m_cmp_buffer[0] & (1u << i)) != 0;
        m_modifiers[i] = (pressed_now && !pressed_bef) || pressed_bef;
    }
    for (uint8_t i = 2; i < 8; i++) {
        const uint8_t key = m_dma_buffer[i];
        if (key_already_pressed(key)) {
            continue;
        }
        const auto &table = m_modifiers[1] ? s_scancode_table_shift : s_scancode_table;
        core::KeyEvent key_event{
            key,
            key < table.size() ? table[key] : '\0',
            m_modifiers[2] || m_modifiers[6],
            m_modifiers[0] || m_modifiers[4],
        };
        m_last_code = key;
        m_last_time = core::time();
        EXPECT(system::syscall(UB_SYS_write, m_pipe.write_fd(), &key_event, sizeof(core::KeyEvent)));
    }
    __builtin_memcpy(m_cmp_buffer.data(), m_dma_buffer, 8);
}
