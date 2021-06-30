#include "APU.hpp"


namespace sb {

APU::APU(Timer &timer, AudioDevice &audio_device) : m_timer(timer), m_audio_device(audio_device), m_sample_counter(CYCLES_PER_SAMPLE) { }

void APU::reset() {
    m_pulse1.reset();
    m_pulse2.reset();
    m_wave.reset();
    m_noise.reset();

    m_nr50 = 0;
    m_nr51 = 0;
}

void APU::write(u16 address, u8 value) {
    if(in_range<u16>(address, 0xFF10, 0xFF14)) {
        m_pulse1.write(address, value);
    } else if(in_range<u16>(address, 0xFF16, 0xFF19)) {
        m_pulse2.write(address, value);
    } else if(in_range<u16>(address, 0xFF1A, 0xFF1E) || in_range<u16>(address, 0xFF30, 0xFF3F)) {
        m_wave.write(address, value);
    } else if(in_range<u16>(address, 0xFF20, 0xFF23)) {
        m_noise.write(address, value);
    }

    switch(address) {
        case 0xFF24 : m_nr50 = value;
        break;
        case 0xFF25 : m_nr51 = value;
        break;
        case 0xFF26 : m_nr52 = value & 0x80;
            //Turning sound off
            if((value & 0x80) == 0) {
                m_pulse1.reset();
                m_pulse2.reset();
                m_wave.reset();
                m_noise.reset();
                reset();
            }
        break;
    }
}

u8 APU::read(u16 address) {
    if(in_range<u16>(address, 0xFF10, 0xFF14)) {
        return m_pulse1.read(address);
    } else if(in_range<u16>(address, 0xFF16, 0xFF19)) {
        return m_pulse2.read(address);
    } else if(in_range<u16>(address, 0xFF1A, 0xFF1E) || in_range<u16>(address, 0xFF30, 0xFF3F)) {
        return m_wave.read(address);
    } else if(in_range<u16>(address, 0xFF20, 0xFF23)) {
        return m_noise.read(address);
    }

    switch(address) {
        case 0xFF24 : return m_nr50;
        case 0xFF25 : return m_nr51;
        case 0xFF26 : return m_nr52 | 0x70 | (m_noise.is_running() << 3) | (m_wave.is_running() << 2) | (m_pulse2.is_running() << 1) | m_pulse1.is_running();
    }

    return 0;
}

void APU::step() {
    //Sound ON/OFF
    if((m_nr52 & 0x80) != 0) {
        //4th bit (zero indexed) of DIV determines when to increment the frame sequencer, equivilent to 8192 T-cycles or 512 Hz
        u8 div = m_timer.read(0xFF04);

        //If falling edge, step the frame sequencer
        if(((m_last_div >> 4) & 1) && !((div >> 4) & 1)) {
            switch(m_fs) {
                case 0 : 
                    m_pulse1.clock_length(); 
                    m_pulse2.clock_length(); 
                    m_wave.clock_length(); 
                    m_noise.clock_length();
                break;
                case 1 :
                break;
                case 2 : 
                    m_pulse1.clock_sweep();
                    m_pulse1.clock_length(); 
                    m_pulse2.clock_length(); 
                    m_wave.clock_length(); 
                    m_noise.clock_length();
                break;
                case 3 :
                break;
                case 4 : 
                    m_pulse1.clock_length(); 
                    m_pulse2.clock_length(); 
                    m_wave.clock_length(); 
                    m_noise.clock_length();
                break;
                case 5 : 
                break;
                case 6 : 
                    m_pulse1.clock_sweep(); 
                    m_pulse1.clock_length(); 
                    m_pulse2.clock_length(); 
                    m_wave.clock_length(); 
                    m_noise.clock_length();
                break;
                case 7 : 
                    m_pulse1.clock_volume(); 
                    m_pulse2.clock_volume();
                    m_noise.clock_volume();
                break;
            }

            m_fs = (m_fs + 1) % 8;
        }

        m_last_div = div;

        //Step channels
        m_pulse1.step();
        m_pulse2.step();
        m_wave.step();
        m_noise.step();
    }

    //Average samples, fixes an aliasing issue (found in Link's Awakening's intro)
    m_average_sample_r += get_so1_sample() / 15.0f; //Put the samples between 0 and 1
    m_average_sample_l += get_so2_sample() / 15.0f;

    if(--m_sample_counter <= 0) {
        //There are bits in the NR50 register that allows the cartridge to 
        //supply a 5th sound channel, but no game ever used it. So I won't implement it.
        float right_vol = m_nr50 & 7;       //Right is sound output terminal 01
        float left_vol = (m_nr50 >> 4) & 7; //Left is sound output terminal 02

        m_average_sample_r /= (float)CYCLES_PER_SAMPLE;
        m_average_sample_l /= (float)CYCLES_PER_SAMPLE;

        m_average_sample_r *= right_vol / 7.0f;
        m_average_sample_l *= left_vol / 7.0f;

        //Put between -1 and 1
        m_average_sample_r = m_average_sample_r * 2 - 1;
        m_average_sample_l = m_average_sample_l * 2 - 1;

        m_audio_device.push_sample(m_average_sample_l * 32767, m_average_sample_r * 32767);
        m_sample_counter = CYCLES_PER_SAMPLE;
        m_average_sample_r = m_average_sample_l = 0;
    }
}

//Mix sample for right speaker
float APU::get_so1_sample() {
    float sample = 0;
    u8 output_channels = 0;
    bool enabled_channels[4];

    for(int i = 0; i < 4; i++) {
        enabled_channels[i] = (m_nr51 >> i) & 1;
    }

    //Pulse Channel 1
    if(enabled_channels[0]) {
        output_channels++;
        sample += m_pulse1.get_amplitude();
    }

    //Pulse Channel 2
    if(enabled_channels[1]) {
        output_channels++;
        sample += m_pulse2.get_amplitude();
    }

    //Wave Channel 3
    if(enabled_channels[2]) {
        output_channels++;
        sample += m_wave.get_amplitude();
    }

    //Noise Channel 4
    if(enabled_channels[3]) {
        output_channels++;
        sample += m_noise.get_amplitude();
    }

    return sample / output_channels;
}

//Mix sample for left speaker
float APU::get_so2_sample() {
    float sample = 0;
    u8 output_channels = 0;
    bool enabled_channels[4];

    for(int i = 0; i < 4; i++) {
        enabled_channels[i] = ((m_nr51 >> 4) >> i) & 1;
    }

    //Pulse Channel 1
    if(enabled_channels[0]) {
        output_channels++;
        sample += m_pulse1.get_amplitude();
    }

    //Pulse Channel 2
    if(enabled_channels[1]) {
        output_channels++;
        sample += m_pulse2.get_amplitude();
    }

    //Wave Channel 3
    if(enabled_channels[2]) {
        output_channels++;
        sample += m_wave.get_amplitude();
    }

    //Noise Channel 4
    if(enabled_channels[3]) {
        output_channels++;
        sample += m_noise.get_amplitude();
    }

    return sample / output_channels;
}

} //namespace sb