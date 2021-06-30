#ifndef APU_HPP
#define APU_HPP

#include "emulator/device/AudioDevice.hpp"
#include "emulator/core/Timer.hpp"
#include "PulseChannel.hpp"
#include "WaveChannel.hpp"
#include "NoiseChannel.hpp"


namespace sb {

constexpr u8 CYCLES_PER_SAMPLE = 95; //4Mhz / 44100 Hz

class APU {
private:

    //Memory Map
    //FF10 | Channel 1 sweep
    //FF11 | Channel 1 Sound length/wave pattern duty
    //FF12 | Channel 1 Volume envelope
    //FF13 | Channel 1 Frequency low
    //FF14 | Channel 1 Frequency high
    //FF15 | ???
    //FF16 | Channel 2 Sound length/wave pattern duty
    //FF17 | Channel 2 Volume envelope
    //FF18 | Channel 2 Frequency low
    //FF19 | Channel 2 Frequency high
    //FF1A | Channel 3 Sound on/off
    //FF1B | Channel 3 Sound length
    //FF1C | Channel 3 Select output level
    //FF1D | Channel 3 Frequency low
    //FF1E | Channel 3 Frequency high
    //FF1F | ???
    //FF20 | Channel 4 Sound length
    //FF21 | Channel 4 Volume envelope
    //FF22 | Channel 4 Polynomial counter
    //FF23 | Channel 4 Counter/consecutive
    //FF24 | Channel control / On-Off / Volume
    //FF25 | Selection of sound output terminal
    //FF26 | Sound on/off
    //FF27 - FF2F | ???
    //FF30 - FF3F | Channel 3 Wave Pattern RAM

    PulseChannel m_pulse1;
    PulseChannel m_pulse2;
    WaveChannel m_wave;
    NoiseChannel m_noise;
    u8 m_nr50;
    u8 m_nr51;
    u8 m_nr52;

    float m_average_sample_l, m_average_sample_r;
    u8 m_sample_counter;
    u8 m_fs; //Frame sequencer
    u8 m_last_div;
    Timer &m_timer;
    AudioDevice &m_audio_device;

public:

    APU(Timer &timer, AudioDevice &audio_device);

    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);

    void step();
    float get_so1_sample();
    float get_so2_sample();
};

} //namespace sb


#endif //APU_HPP