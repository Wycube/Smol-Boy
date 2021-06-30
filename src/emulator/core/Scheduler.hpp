#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "common/Types.hpp"

#include <functional>


namespace sb {

class CPU;
class PPU;


class Clock {
private:

    usize m_t_count = 0;
    usize m_m_count = 0;

public:

    void reset() { m_t_count = 0; m_m_count = 0; }
    void add_t(usize cycles) { m_t_count += cycles; m_m_count = m_t_count / 4; }
    void add_m(usize cycles) { m_m_count += cycles; m_t_count = m_m_count * 4; }
    usize get_t() { return m_t_count; }
    usize get_m() { return m_m_count; }
};


using StepFunction = std::function<void()>;

//A simple relative scheduler that tries to keep the cpu and ppu in sync cycles-wise.
class Scheduler {
private:

    StepFunction m_cpu_step;
    StepFunction m_ppu_step;

    void reset_clocks();

public:

    Clock cpu_clock;
    Clock ppu_clock;

    void reset();
    void run_for(usize cycles);

    void set_cpu_step(StepFunction cpu_step) { m_cpu_step = cpu_step; }
    void set_ppu_step(StepFunction ppu_step) { m_ppu_step = ppu_step; }
};

} //namespace sb


#endif //SCHEDULER_HPP