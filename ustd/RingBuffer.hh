#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace ustd {

template <typename T, usize Capacity>
class RingBuffer {
    Array<T, Capacity> m_data{};
    usize m_head{0};
    usize m_size{0};

public:
    void enqueue(T elem);
    T dequeue();

    bool empty() const { return m_size == 0; }
};

template <typename T, usize Capacity>
void RingBuffer<T, Capacity>::enqueue(T elem) {
    m_data[(m_head + m_size) % Capacity] = elem;
    if (m_size == Capacity) {
        m_head = (m_head + 1) % Capacity;
    } else {
        m_size++;
    }
}

template <typename T, usize Capacity>
T RingBuffer<T, Capacity>::dequeue() {
    T elem = m_data[m_head];
    m_head = (m_head + 1) % Capacity;
    m_size--;
    return elem;
}

} // namespace ustd
