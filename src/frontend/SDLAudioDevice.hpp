#ifndef SDL_AUDIO_DEVICE_HPP
#define SDL_AUDIO_DEVICE_HPP

#include "emulator/device/AudioDevice.hpp"
#include "common/Log.hpp"

#include <SDL.h>


class SDLAudioDevice : public sb::AudioDevice {
private:

    SDL_AudioSpec m_obtained_spec;
    SDL_AudioDeviceID m_device_id;

public:

    SDLAudioDevice() {
        //Define desired specs
        SDL_AudioSpec desired;
        desired.freq = SAMPLE_RATE;
        desired.channels = 2;
        desired.format = AUDIO_S16SYS;
        desired.samples = SAMPLE_BUFFER_SIZE;
        desired.callback = nullptr;
        desired.userdata = this;

        //Open an audio device
        m_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired, &m_obtained_spec, 0);

        if(desired.format != m_obtained_spec.format) {
            LOG_FATAL("[Audio] : Didn't get the requested Signed 16-bit Format!");
        }

        if(m_device_id == 0) {
            LOG_FATAL("[Audio] : SDL failed to open an audio device! Error: {}", SDL_GetError());
        }

        //Start playing
        SDL_PauseAudioDevice(m_device_id, 0);
    }

    ~SDLAudioDevice() {
        //Stop playing and close device
        SDL_PauseAudioDevice(m_device_id, 1);
        SDL_CloseAudioDevice(m_device_id);
    }

    void queue_audio() override {
        //Copy data from ring buffer into an array.
        //The ring buffer should have all 1024 samples, since
        //this function wouldn't be caled otherwise
        s16 samples[SAMPLE_BUFFER_SIZE];
        for(usize i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
            samples[i] = m_sample_buffer.pop_front();
        }

        m_sample_buffer.clear();
        
        //Delay in order to synchronize the emulator
        while(SDL_GetQueuedAudioSize(m_device_id) > SAMPLE_BUFFER_SIZE * sizeof(s16)) {
            SDL_Delay(1);
        }

        SDL_QueueAudio(m_device_id, samples, SAMPLE_BUFFER_SIZE * sizeof(s16));
    }
};


#endif //SDL_AUDIO_DEVICE_HPP