#ifndef NOISE_CHANNEL_HPP
#define NOISE_CHANNEL_HPP

#include "common/Types.hpp"


namespace sb {

//Covers channel 4
class NoiseChannel {
private:

    //Registers
    u8 m_nr41; //FF20 | Sound length
    u8 m_nr42; //FF21 | Volumne envelope
    u8 m_nr43; //FF22 | Polynomial counter
    u8 m_nr44; //FF23 | Counter/consecutive

    u16 m_lfsr; //Linear Feedback Shift Register
    u16 m_freq_timer;
    u8 m_length_timer;
    u8 m_period_timer;
    u8 m_current_vol;
    u8 m_output_vol;
    bool m_enabled;
    bool m_dac_enabled;

    void trigger();

public:

    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);

    void step();
    void clock_volume();
    void clock_length();

    u8 get_amplitude();
    bool is_running();
};

} //namespace sb


#endif //NOISE_CHANNEL_HPP