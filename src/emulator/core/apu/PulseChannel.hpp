#ifndef PULSE_CHANNEL_HPP
#define PULSE_CHANNEL_HPP

#include "common/Types.hpp"


namespace sb {

//Covers channels 1 and 2, except 2 doesn't have sweep
class PulseChannel {
private:

    //Registers
    u8 m_nr10; //FF10         | Channel 1 sweep, channel 2 doesn't support sweep
    u8 m_nrx1; //FF11 or FF16 | Sound length / wave pattern duty
    u8 m_nrx2; //FF12 or FF17 | Volume Envelope
    u8 m_nrx3; //FF13 or FF18 | Frequency low
    u8 m_nrx4; //FF14 or FF19 | Frequency high

    u8 m_wave_duty_pos;
    u16 m_freq_timer;
    u8 m_length_timer;
    u8 m_period_timer;
    u8 m_current_vol;
    u8 m_output_vol;
    u16 m_shadow_freq;
    u8 m_sweep_timer;
    bool m_sweep_enabled;
    bool m_enabled;
    bool m_dac_enabled;

    void trigger();
    u16 calculate_freq();

public:

    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);

    void step();
    void clock_sweep();
    void clock_volume();
    void clock_length();

    u8 get_amplitude();
    bool is_running();
};

} //namespace sb


#endif //PULSE_CHANNEL_HPP