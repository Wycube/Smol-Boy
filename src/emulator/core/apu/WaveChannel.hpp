#ifndef WAVE_CHANNEL_HPP
#define WAVE_CHANNEL_HPP

#include "common/Types.hpp"


namespace sb {

//Covers channel 3, which uses custom waveforms
class WaveChannel {
private:

    //Registers and Wave Pattern Data
    u8 m_nr30;         //FF1A | Sound on/off
    u8 m_nr31;         //FF1B | Sound length
    u8 m_nr32;         //FF1C | Select output level
    u8 m_nr33;         //FF1D | Frequency low
    u8 m_nr34;         //FF1E | Frequency high
    u8 m_wave_ram[16]; //FF30 - FF3F | Wave Pattern RAM

    u8 m_wave_pos;
    u16 m_freq_timer;
    u16 m_length_timer;
    u8 m_volume_shift;
    bool m_enabled;
    bool m_dac_enabled;

    void trigger();

public:

    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);

    void step();
    void clock_length();

    u8 get_amplitude();
    bool is_running();
};

} //namespace sb


#endif //WAVE_CHANNEL_HPP