#ifndef AUDIO_DEVICE_HPP
#define AUDIO_DEVICE_HPP

#include "common/Types.hpp"
#include "common/Utility.hpp"


namespace sb {

class AudioDevice {
protected:

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr usize SAMPLE_BUFFER_SIZE = 1024;
    RingBuffer<s16, SAMPLE_BUFFER_SIZE * 2 + 1> m_sample_buffer; //+1 so the ring buffer doesn't report empty when it's size reaches 1024 * 2

public:

    void push_sample(s16 left, s16 right) {
        //Samples are interleaved LRLRLR... for stereo
        m_sample_buffer.push_back(left);
        m_sample_buffer.push_back(right);
    }
};


class NullAudioDevice : public AudioDevice {
private:

};

} //namespace sb


#endif //AUDIO_DEVICE_HPP