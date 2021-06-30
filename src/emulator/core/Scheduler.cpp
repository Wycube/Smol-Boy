#include "Scheduler.hpp"


namespace sb {

//Determines which clock is lower and resets that to zero, then resets the other plus the difference
void Scheduler::reset_clocks() {
    if(cpu_clock.get_t() <= ppu_clock.get_t()) {
        usize diff = ppu_clock.get_t() - cpu_clock.get_t();
        cpu_clock.reset();
        ppu_clock.reset();
        ppu_clock.add_t(diff);
    } else {
        usize diff = cpu_clock.get_t() - ppu_clock.get_t();
        ppu_clock.reset();
        cpu_clock.reset();
        cpu_clock.add_t(diff);
    }
}

void Scheduler::reset() {
    cpu_clock.reset();
    ppu_clock.reset();
}

//Run for at least that amount of cycles, it might go past it a bit
void Scheduler::run_for(usize cycles) {
    usize target = cpu_clock.get_t() + cycles;

    while(cpu_clock.get_t() < target) {
        //Determine which part is behind
        if(cpu_clock.get_t() <= ppu_clock.get_t()) {
            m_cpu_step();
        } else {
            m_ppu_step();
        }
    }

    reset_clocks();
}

} //namespace sb