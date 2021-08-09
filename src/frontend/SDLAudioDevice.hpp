#ifndef SDL_AUDIO_DEVICE_HPP
#define SDL_AUDIO_DEVICE_HPP

#include "emulator/device/AudioDevice.hpp"
#include "common/Log.hpp"

#include <SDL.h>
#include <mutex>


static constexpr int CYCLES_PER_AUDIO_FRAME = 97200; //95 cycles per sample * 1024 samples (stereo)

namespace sb {
    class Gameboy;
}

class SDLAudioDevice : public sb::AudioDevice {
private:

    SDL_AudioSpec m_obtained_spec;
    SDL_AudioDeviceID m_device_id;

    sb::Gameboy *m_emu;
    bool m_sync_to_audio;
    float m_last_frametime;

public:

    SDLAudioDevice() {
        //Define desired specs
        SDL_AudioSpec desired;
        desired.freq = SAMPLE_RATE;
        desired.channels = 2;
        desired.format = AUDIO_S16SYS;
        desired.samples = SAMPLE_BUFFER_SIZE;
        desired.callback = callback;
        desired.userdata = this;

        //Open an audio device
        m_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired, &m_obtained_spec, 0);

        if(desired.format != m_obtained_spec.format) {
            LOG_FATAL("[SDLAudio] : Didn't get the requested Signed 16-bit Format!");
        }

        if(m_device_id == 0) {
            LOG_FATAL("[SDLAudio] : SDL failed to open an audio device! Error: {}", SDL_GetError());
        }
    }

    ~SDLAudioDevice() {
        //Stop playing and close device
        SDL_PauseAudioDevice(m_device_id, 1);
        SDL_CloseAudioDevice(m_device_id);
    }

    void start() {
        SDL_PauseAudioDevice(m_device_id, 0);
    }

    void stop() {
        SDL_PauseAudioDevice(m_device_id, 1);
    }

    void set_sync(bool value, sb::Gameboy *emu = nullptr) {
        m_sync_to_audio = value;
        
        if(value && emu == nullptr) {
            LOG_FATAL("[SDLAudio] : Cannot sync to null Gameboy pointer!");
        } else {
            m_emu = emu;
        }
    }

    bool syncing() {
        return m_sync_to_audio;
    }

    float last_frametime() {
        return m_last_frametime;
    }

    static u16 resample_linear(u16 *buffer, int buffer_size, int sample_index, int num_samples) {
        //Get the location within the number of samples between 0 and 1
        float location = (float)sample_index / (float)num_samples;
    
        //Get the location within the buffer of samples, then get the top and bottom integer indexes from it
        float buffer_loc = location * buffer_size;
        int loc_top = std::ceil(buffer_loc);
        int loc_bot = std::floor(buffer_loc);
        float fractional = buffer_loc - loc_bot;

        u16 top_sample = buffer[loc_top];
        u16 bot_sample = buffer[loc_bot];

        //Linearly interpolate the samples, based on the fractional part of the location, and return the result
        u16 new_sample = bot_sample * fractional + top_sample * (1.0f - fractional);

        return new_sample;
    }

    static void callback(void *user_data, u8 *stream, int length) {
        SDLAudioDevice *device = reinterpret_cast<SDLAudioDevice*>(user_data);
        u16 *_stream = reinterpret_cast<u16*>(stream);

        memset(_stream, 0, length);

        //Run the gameboy until the audio buffer is full, if sync_to_audio is enabled
        if(device->m_sync_to_audio) {
            while(device->m_sample_buffer.size() < SAMPLE_BUFFER_SIZE * 2) {
                device->m_emu->run_for(75);
            }


            //Fill audio stream with samples, length divided by two because it is in bytes
            //and the samples are two bytes long, and i is incremented by two because each sample is stereo, Left and Right samples.
            for(int i = 0; i < length / 2; i += 2) {
                u16 sample_left = device->m_sample_buffer.pop_front();
                u16 sample_right = device->m_sample_buffer.pop_front();

                _stream[i] = sample_left;
                _stream[i + 1] = sample_right;
            }
        } else {
            int new_length = length > device->m_sample_buffer.size() ? device->m_sample_buffer.size() : length;

            //Fill arrays with left and right samples
            int size = device->m_sample_buffer.size() / 2;
            u16 *left_samples = new u16[size];
            u16 *right_samples = new u16[size];
            
            for(size_t i = 0; i < size; i++) {
                left_samples[i] = device->m_sample_buffer.pop_front();
                right_samples[i] = device->m_sample_buffer.pop_front();
            }

            for(int i = 0; i < new_length / 2; i += 2) {
                u16 sample_left = resample_linear(left_samples, size, i / 2, SAMPLE_BUFFER_SIZE);
                u16 sample_right = resample_linear(right_samples, size, i / 2, SAMPLE_BUFFER_SIZE);

                _stream[i] = sample_left;
                _stream[i + 1] = sample_right;
            }
        }


        device->m_sample_buffer.clear();
    }
};


#endif //SDL_AUDIO_DEVICE_HPP