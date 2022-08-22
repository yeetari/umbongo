#pragma once

#include <core/File.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

class Drive {
    core::File m_file;
    usize m_block_size{0};

public:
    explicit Drive(core::File &&file) : m_file(ustd::move(file)) {}

    void set_block_size(usize block_size) { m_block_size = block_size; }

    template <typename T>
    T read(usize block) {
        T ret;
        EXPECT(m_file.read({&ret, sizeof(T)}, block * m_block_size));
        return ret;
    }

    usize read(ustd::Span<void> data, usize block, usize initial_offset = 0) {
        usize bytes_read = 0;
        while (bytes_read < data.size()) {
            const auto offset = block * m_block_size + bytes_read + initial_offset;
            bytes_read += EXPECT(m_file.read({&data.as<char>()[bytes_read], data.size() - bytes_read}, offset));
        }
        return bytes_read;
    }
};
