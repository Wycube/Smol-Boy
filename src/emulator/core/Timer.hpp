#ifndef TIMER_HPP
#define TIMER_HPP

#include "common/Types.hpp"
#include "cpu/CPU.hpp"

#include <chrono>


namespace sb {

class Timer {
private:

    u16 m_old_internal_counter;
    u16 m_internal_counter; //Div is the upper 8-bits of this internal counter
    bool m_tima_overflow;
    u8 m_overflow_count;
    u8 m_tima;
    u8 m_tma;
    u8 m_tac;

    CPU &m_cpu;

public:

    Timer(CPU &cpu);

    void step();
    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);
};

} //namespace sb


#endif //TIMER_HPP