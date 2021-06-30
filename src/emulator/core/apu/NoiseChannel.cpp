#include "NoiseChannel.hpp"
#include "common/Log.hpp"


namespace sb {

static constexpr u8 DIVISORS[8] = {8, 16, 32, 48, 64, 80, 96, 112};


void NoiseChannel::reset() {
    m_enabled = false;
    m_dac_enabled = false;
    m_nr41 = 0;
    m_nr42 = 0;
    m_nr43 = 0;
    m_nr44 = 0;
}

void NoiseChannel::write(u16 address, u8 value) {
    switch(address) {
        case 0xFF20 : m_nr41 = value;
            //Reload length timer
            m_length_timer = 64 - (value & 63);
        break;
        case 0xFF21 : m_nr42 = value;
            //Check if DAC is enabled (All 5 high bits are not zero)
            m_dac_enabled = (value & 0xF8) != 0;
        break;
        case 0xFF22 : m_nr43 = value;
        break;
        case 0xFF23 : m_nr44 = value;
            //Initial set, restart sound
            if(value >> 7) {
                trigger();
            }
        break;
    }
}

u8 NoiseChannel::read(u16 address) {
    switch(address) {
        case 0xFF20 : return m_nr41 | 0xff;
        case 0xFF21 : return m_nr42;
        case 0xFF22 : return m_nr43;
        case 0xFF23 : return m_nr44 | 0xbf;
    }

    return 0xff;
}

void NoiseChannel::trigger() {
    m_enabled = true;

    if(m_length_timer == 0) {
        m_length_timer = 64;
    }

    //LFSR is reset to all ones, it is 15-bits wide
    m_lfsr = 0x7fff;

    m_freq_timer = DIVISORS[m_nr43 & 7] << (m_nr43 >> 4);

    //Set volume envelope variables
    m_current_vol = m_nr42 >> 4;
    m_period_timer = m_nr42 & 7;
}

void NoiseChannel::step() {
    if(m_enabled && m_dac_enabled) {
        m_output_vol = m_current_vol;
    } else {
        m_output_vol = 0;
    }

    if(--m_freq_timer <= 0) {
        m_freq_timer = DIVISORS[m_nr43 & 7] << (m_nr43 >> 4);

        //Some weird stuff to make the white noise
        u8 xor_result = (m_lfsr & 1) ^ ((m_lfsr >> 1) & 1); //XOR first two bits
        m_lfsr = (m_lfsr >> 1) | (xor_result << 14); //Store XOR-result into the 14th bit

        //This makes the noise more regular, to resemble a tone more
        if((m_nr43 >> 3) & 1) {
            m_lfsr &= ~(1 << 6);
            m_lfsr |= xor_result << 6; //Also store the XOR-result into the 6th bit
        }


    }
}

void NoiseChannel::clock_volume() {
    //Period must not be zero
    if((m_nr42 & 7) != 0) {
        if(m_period_timer != 0) {
            m_period_timer--;
        }

        if(m_period_timer == 0) {
            m_period_timer = (m_nr42 & 7) == 0 ? 8 : m_nr42 & 7; //Weird behavior

            //Do checks and changes based on envelope direction
            if((m_nr42 >> 3) & 1) { //Upward / Increase
                if(m_current_vol < 0xf) {
                    m_current_vol++;
                }
            } else { //Downward / Decrease
                if(m_current_vol > 0) {
                    m_current_vol--;
                }
            }
        }
    }
}

void NoiseChannel::clock_length() {
    if(!((m_nr44 >> 6) & 1) || m_length_timer <= 0) {
        return;
    }

    if(--m_length_timer == 0) {
        m_enabled = false;
    }
}

u8 NoiseChannel::get_amplitude() {
    return (~m_lfsr & 1) * m_output_vol;
}

bool NoiseChannel::is_running() {
    return m_enabled && m_dac_enabled;
}

} //namespace sb