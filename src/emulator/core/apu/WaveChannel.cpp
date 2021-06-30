#include "WaveChannel.hpp"
#include "common/Utility.hpp"


namespace sb {

static constexpr u8 VOLUME_SHIFTS[4] = {4, 0, 1, 2};


void WaveChannel::reset() {
    m_enabled = false;
    m_dac_enabled = false;
    m_nr30 = 0;
    m_nr31 = 0;
    m_nr32 = 0;
    m_nr33 = 0;
    m_nr34 = 0;
}

void WaveChannel::write(u16 address, u8 value) {
    switch(address) {
        case 0xFF1A : m_nr30 = value;
            //Sound on/off
            m_dac_enabled = value >> 7;
        break;
        case 0xFF1B : m_nr31 = value;
            //Reload length timer
            m_length_timer = 256 - value;
        break;
        case 0xFF1C : m_nr32 = value;
        break;
        case 0xFF1D : m_nr33 = value;
        break;
        case 0xFF1E : m_nr34 = value;
            //Initial set, restart sound
            if((value >> 7) & 1) {
                trigger();
            }
        break;
    }

    if(in_range<u16>(address, 0xFF30, 0xFF3F)) {
        m_wave_ram[address - 0xFF30] = value;
    }
}

u8 WaveChannel::read(u16 address) {
    switch(address) {
        case 0xFF1A : return m_nr30 | 0x7f;
        case 0xFF1B : return m_nr31 | 0xff;
        case 0xFF1C : return m_nr32 | 0x9f;
        case 0xFF1D : return m_nr33 | 0xff;
        case 0xFF1E : return m_nr34 | 0xbf;
    }

    if(in_range<u16>(address, 0xFF30, 0xFF3F)) {
        return m_wave_ram[address - 0xFF30];
    }

    return 0xff;
}

void WaveChannel::trigger() {
    m_enabled = true;

    if(m_length_timer == 0) {
        m_length_timer = 256;
    }

    m_freq_timer = (2048 - (m_nr33 | ((m_nr34 & 7) << 8))) * 2;
    m_wave_pos = 0;
}

void WaveChannel::step() {
    //Don't output sound when disabled
    if(m_enabled && m_dac_enabled) {
        m_volume_shift = VOLUME_SHIFTS[(m_nr32 >> 5) & 3];
    } else {
        m_volume_shift = 4;
    }

    if(--m_freq_timer <= 0) {
        u16 frequency = m_nr33 | ((m_nr34 & 7) << 8);
        m_freq_timer = (2048 - frequency) * 2;
        m_wave_pos = (m_wave_pos + 1) % 32;
    }
}

void WaveChannel::clock_length() {
    if(!((m_nr34 >> 6) & 1) || m_length_timer <= 0) {
        return;
    }

    if(--m_length_timer <= 0) {
        m_enabled = false;
    }
}

u8 WaveChannel::get_amplitude() {
    //Get upper nibble or lower nibble based on wave RAM index
    u8 amplitude;

    if(m_wave_pos % 2 == 0) { //Upper nibble
        amplitude = (m_wave_ram[m_wave_pos / 2] & 0xf0) >> 4;
    } else { //Lower nibble
        amplitude = m_wave_ram[m_wave_pos / 2] & 0xf;
    }

    return amplitude >> m_volume_shift;
}

bool WaveChannel::is_running() {
    return m_enabled && m_dac_enabled;
}

} //namespace sb