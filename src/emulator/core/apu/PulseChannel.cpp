#include "PulseChannel.hpp"
#include "common/Log.hpp"


namespace sb {

//1 for peaks and 0 for troughs     12.5%       25%         50%         75%
static constexpr u8 WAVE_DUTY[4] = {0b00000001, 0b10000001, 0b10000111, 0b01111110};


void PulseChannel::reset() {
    m_enabled = false;
    m_dac_enabled = false;
    m_nr10 = 0;
    m_nrx1 = 0;
    m_nrx2 = 0;
    m_nrx3 = 0;
    m_nrx4 = 0;
}

void PulseChannel::write(u16 address, u8 value) {
    switch(address) {
        case 0xFF10 : m_nr10 = value;
        break;
        case 0xFF11 :
        case 0xFF16 : m_nrx1 = value;
            //Reload length timer
            m_length_timer = 64 - (value & 63);
        break;
        case 0xFF12 :
        case 0xFF17 : m_nrx2 = value;
            //Check if DAC enabled
            m_dac_enabled = (value & 0xF8) != 0;
        break;
        case 0xFF13 :
        case 0xFF18 : m_nrx3 = value;
        break;
        case 0xFF14 :
        case 0xFF19 : m_nrx4 = value;
            //Initial set, restart sound
            if(value >> 7) {
                trigger();
            }
        break;
    }
}

u8 PulseChannel::read(u16 address) {
    //Last written values are ORed with certain numbers, including 0
    switch(address) {
        case 0xFF10 : return m_nr10 | 0x80;
        case 0xFF11 :
        case 0xFF16 : return m_nrx1 | 0x3f;
        case 0xFF12 :
        case 0xFF17 : return m_nrx2;
        case 0xFF13 :
        case 0xFF18 : return m_nrx3 | 0xff;
        case 0xFF14 : 
        case 0xFF19 : return m_nrx4 | 0xbf;
    }

    return 0xff;
}

void PulseChannel::trigger() {
    m_enabled = true;

    if(m_length_timer == 0) {
        m_length_timer = 64;
    }

    m_freq_timer = (2048 - (m_nrx3 | ((m_nrx4 & 7) << 8))) * 4;

    //Set volume envelope variables
    m_current_vol = m_nrx2 >> 4;
    m_period_timer = m_nrx2 & 7;

    //Set sweep variables
    m_shadow_freq = m_nrx3 | ((m_nrx4 & 7) << 8);
    
    u8 sweep_period = (m_nr10 >> 4) & 7;
    m_sweep_timer = sweep_period == 0 ? 8 : sweep_period;

    //Enabled if sweep period is not zero OR sweep shift is not zero
    m_sweep_enabled = (sweep_period != 0) || (m_nr10 & 7);

    //If sweep shift is not equal to zero, run the overflow check via the calculate_freq method
    if((m_nr10 & 7) != 0) {
        calculate_freq();
    }
}

//Calculates a new frequency used for sweep, and performs an overflow check
u16 PulseChannel::calculate_freq() {
    //Shift the shadow frequency based on the value in NR10
    u16 new_freq = m_shadow_freq >> (m_nr10 & 7); 

    //Calculate new frequency based on sweep direction
    if((m_nr10 >> 3) & 1) { //Decreasing
        new_freq = m_shadow_freq - new_freq;
    } else { //Increasing
        new_freq = m_shadow_freq + new_freq;
    }

    //Overflow check
    if(new_freq > 2047) {
        m_enabled = false;
    }

    return new_freq;
}

void PulseChannel::step() {
    //Don't output sound when disabled
    if(m_enabled && m_dac_enabled) {
        m_output_vol = m_current_vol;
    } else {
        m_output_vol = 0;
    }

    if(--m_freq_timer <= 0) {
        u16 frequency = m_nrx3 | ((m_nrx4 & 7) << 8);
        m_freq_timer = (2048 - frequency) * 4;
        m_wave_duty_pos = (m_wave_duty_pos + 1) % 8;
    }
}

void PulseChannel::clock_sweep() {
    if(m_sweep_timer > 0) {
        m_sweep_timer--;

        if(m_sweep_timer == 0) {
            //If the sweep period is zero, reload the timer with 8 instead
            u8 sweep_period = (m_nr10 >> 4) & 7;
            m_sweep_timer = sweep_period == 0 ? 8 : sweep_period;

            if(m_sweep_enabled && sweep_period != 0) {
                //Calculate new frequency and perform overflow check
                u16 new_freq = calculate_freq();

                if(new_freq < 2048 && (m_nr10 & 7) != 0) {
                    m_shadow_freq = new_freq;
                    
                    //Store in the frequency registers as well, only lower 3 bits of NR14
                    m_nrx3 = new_freq & 0xff;
                    m_nrx4 &= ~7;
                    m_nrx4 |= (new_freq >> 8) & 7;
                
                    //This does the overflow check again
                    calculate_freq();
                }
            }
        }
    }
}

void PulseChannel::clock_volume() {
    //Period must not be zero
    if((m_nrx2 & 7) != 0) {
        if(m_period_timer != 0) {
            m_period_timer--;
        }

        if(m_period_timer == 0) {
            m_period_timer = (m_nrx2 & 7) == 0 ? 8 : m_nrx2 & 7; //Weird behavior

            //Do checks and changes based on envelope direction
            if((m_nrx2 >> 3) & 1) { //Upward / Increase
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

void PulseChannel::clock_length() {
    if(!((m_nrx4 >> 6) & 1) || m_length_timer <= 0) {
        return;
    }

    if(--m_length_timer <= 0) {
        m_enabled = false;
    }
}

u8 PulseChannel::get_amplitude() {
    return m_output_vol * ((WAVE_DUTY[m_nrx1 >> 6] >> (7 - m_wave_duty_pos)) & 1);
}

bool PulseChannel::is_running() {
    return m_enabled && m_dac_enabled;
}

} //namespace sb