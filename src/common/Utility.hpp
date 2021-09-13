#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "Types.hpp"

#include <string_view>
#include <stdexcept>
#include <mutex>


constexpr u16 KiB = 1024;

inline constexpr std::string_view base_name(const std::string_view &path, const std::string_view &delims = "/\\") {
	return path.substr(path.find_last_of(delims) + 1);
}

inline constexpr std::string_view file_name(const std::string_view &name) {
	return name.substr(0, name.find_last_of('.'));
}

inline constexpr u16 to_u16(u8 high, u8 low) {
	return (high << 8) | low;
}

template<typename T>
inline constexpr bool in_range(T value, T min, T max) {
	return value >= min  && value <= max;
}

template<class T, usize _capacity>
class RingBuffer {
private:

	T m_buffer[_capacity];
	usize m_head, m_tail;
	usize m_size;
	std::mutex m_lock;

public:

	RingBuffer() : m_head(0), m_tail(0) { }

	void push_back(T value) {
		m_lock.lock();
		m_buffer[m_head] = value;
		m_head = (m_head + 1) % _capacity;
		m_lock.unlock();
	}

	T pop_front() {
		//Check if it's empty
		if(m_head == m_tail) {
			throw new std::out_of_range("Trying to pop front of an empty RingBuffer!");
		}

		m_lock.lock();
		T &elem = m_buffer[m_tail];
		m_tail = (m_tail + 1) % _capacity;
		m_lock.unlock();

		return elem;
	}

	void clear() {
		m_lock.lock();
		m_head = 0;
		m_tail = 0;
		m_buffer[0] = 0;
		m_lock.unlock();
	}

	usize capacity() {
		return _capacity;
	}

	usize size() {
		return m_head >= m_tail ? m_head - m_tail : (_capacity - m_tail) + m_head;
	}
};


#endif //UTILITY_HPP