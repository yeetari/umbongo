#pragma once

#include <ustd/array.hh>
#include <ustd/types.hh>

namespace ustd {

template <typename T, size_t Capacity>
class RingBuffer {
    Array<T, Capacity> m_data{};
    size_t m_head{0};
    size_t m_size{0};

public:
    void enqueue(T elem);
    T dequeue();

    bool empty() const { return m_size == 0; }
    size_t head() const { return m_head; }
    size_t size() const { return m_size; }
    const T &operator[](size_t index) const { return m_data[index]; }
};

template <typename T, size_t Capacity>
void RingBuffer<T, Capacity>::enqueue(T elem) {
    m_data[(m_head + m_size) % Capacity] = elem;
    if (m_size == Capacity) {
        m_head = (m_head + 1) % Capacity;
    } else {
        m_size++;
    }
}

template <typename T, size_t Capacity>
T RingBuffer<T, Capacity>::dequeue() {
    T elem = m_data[m_head];
    m_head = (m_head + 1) % Capacity;
    m_size--;
    return elem;
}

} // namespace ustd
