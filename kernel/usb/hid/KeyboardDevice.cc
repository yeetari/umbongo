#include <kernel/usb/hid/KeyboardDevice.hh>

#include <kernel/usb/Descriptors.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace usb {
namespace {

const Array s_scancode_table{
    '\0', '\0', '\0', '\0', 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',
    'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2',  '3',  '4',
    '5',  '6',  '7',  '8',  '9',  '0',  '\n', '\0', '\b', '\t', ' ',  '-',  '=',  '[',  ']',  '#',  '\0',
    ';',  '\'', '`',  ',',  '.',  '/',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\\',
};

const Array s_scancode_table_shift{
    '\0', '\0', '\0', '\0', 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',
    'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '"',  '\0', '$',
    '%',  '^',  '&',  '*',  '(',  ')',  '\0', '\0', '\0', '\0', '\0', '_',  '+',  '{',  '}',  '~',  '\0',
    ':',  '@',  '\0', '<',  '>',  '?',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '|',
};

} // namespace

KeyboardDevice::KeyboardDevice(Device &&device) : Device(ustd::move(device)) {
    walk_configuration([this](void *descriptor, DescriptorType type) {
        if (type == DescriptorType::Configuration) {
            auto *config_descriptor = static_cast<ConfigDescriptor *>(descriptor);
            set_configuration(config_descriptor->config_value);
        } else if (type == DescriptorType::Endpoint) {
            auto *endpoint_descriptor = static_cast<EndpointDescriptor *>(descriptor);
            if ((endpoint_descriptor->address & (1u << 7u)) == 0) {
                return;
            }
            m_input_endpoint = create_endpoint(endpoint_descriptor->address);
            m_input_endpoint->setup(EndpointType::InterruptIn, endpoint_descriptor->packet_size);
            m_input_endpoint->setup_interval_input(m_buffer, endpoint_descriptor->interval);
            configure_endpoint(m_input_endpoint);
        }
    });
}

bool KeyboardDevice::key_already_pressed(uint8 key) const {
    for (uint8 i = 2; i < 8; i++) {
        if (key == m_compare_buffer[i]) {
            return true;
        }
    }
    return false;
}

void KeyboardDevice::poll() {
    for (uint8 i = 0; i < 8; i++) {
        bool pressed_now = (m_buffer[0] & (1u << i)) != 0;
        bool pressed_bef = (m_compare_buffer[0] & (1u << i)) != 0;
        m_modifiers[i] = (pressed_now && !pressed_bef) || pressed_bef;
    }
    for (uint8 i = 2; i < 8; i++) {
        const uint8 key = m_buffer[i];
        if (key_already_pressed(key)) {
            continue;
        }
        const auto &table = m_modifiers[1] ? s_scancode_table_shift : s_scancode_table;
        const auto ch = static_cast<uint8>(table[key]);
        if (ch != '\0') {
            log("{:c}", ch);
        }
    }
    memcpy(m_compare_buffer.data(), m_buffer.data(), m_buffer.size());
}

} // namespace usb
