#include "Timer.hpp"

#define GB_CLOCK_FREQ 4194304


namespace sb {

static constexpr u32 cycles_per_tick[4] = {GB_CLOCK_FREQ / 4096, GB_CLOCK_FREQ / 262144, GB_CLOCK_FREQ / 65536, GB_CLOCK_FREQ / 16384};

Timer::Timer(CPU &cpu) : m_cpu(cpu) {
    reset();
}

void Timer::step() {
    m_old_internal_counter = m_internal_counter;
    m_internal_counter++;

    //4-cycle delay where tima reads 00
    m_overflow_count++;
    if(m_overflow_count > 3) {
        m_overflow_count = 0;
        m_tima_overflow = false;
    }

    //Increment 
    const u8 counter_bits[4] = {9, 3, 5, 7};
    bool old_bit = m_old_internal_counter >> (counter_bits[m_tac & 3]) & 1;
    bool current_bit = m_internal_counter >> (counter_bits[m_tac & 3]) & 1;
    if((m_tac >> 2 & 1) && old_bit && !current_bit) {
        m_tima++;

        if(m_tima == 0) {
            m_tima_overflow = true;
            m_overflow_count = 0;

            m_tima = m_tma;
            m_cpu.request_interrupt(TIMER_INT);
        }
    }
}

void Timer::reset() {
    m_internal_counter = 0;
    m_tima = 0;
    m_tma = 0;
    m_tac = 0;
}

void Timer::write(u16 address, u8 value) {
    u8 old;
    
    switch(address) {
        case 0xFF04 : m_internal_counter = 0;
        break;
        case 0xFF05 : m_tima = value;
        break;
        case 0xFF06 : m_tma = value;
        break;
        case 0xFF07 : 
        old = m_tac;
        m_tac = value; m_tac |= 0b11111000;
        break;
        default : break;
    }
}

u8 Timer::read(u16 address) {
    switch(address) {
        case 0xFF04 : return m_internal_counter >> 8;
        case 0xFF05 : if(!m_tima_overflow) return m_tima; else return 0;
        case 0xFF06 : return m_tma;
        case 0xFF07 : return m_tac;
        default : return 0xff;
    }
}

} //namespace sb