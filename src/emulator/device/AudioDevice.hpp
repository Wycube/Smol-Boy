#ifndef AUDIO_DEVICE_HPP
#define AUDIO_DEVICE_HPP

#include "common/Types.hpp"
#include "common/Utility.hpp"


namespace sb {

class AudioDevice {
protected:

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr usize SAMPLE_BUFFER_SIZE = 1024;
    RingBuffer<s16, SAMPLE_BUFFER_SIZE + 1> m_sample_buffer; //+1 so the ring buffer doesn't report empty when it's size reaches 1024

    virtual void queue_audio() = 0;

public:

    void push_sample(s16 left, s16 right) {
        //Samples are interleaved LRLRLR... for stereo
        m_sample_buffer.push_back(left);
        m_sample_buffer.push_back(right);

        if(m_sample_buffer.size() >= SAMPLE_BUFFER_SIZE) {
            queue_audio(); //This will queue the audio for whatever backend, and pause until only one buffer full is left for synchronization
        }
    }
};


class NullAudioDevice : public AudioDevice {
private:

    void queue_audio() override { }

};

} //namespace sb


#endif //AUDIO_DEVICE_HPP