#ifndef GB_COMMON
#define GB_COMMON

#include "emulator/device/VideoDevice.hpp"
#include "emulator/device/InputDevice.hpp"
#include "emulator/device/AudioDevice.hpp"


namespace sb {

enum GB_MODEL {
    DMG, //Dot Matrix Game
    CGB  //Color Game Boy
};


//So the constructor doesn't have to be so long
struct GameboySettings {
    VideoDevice &video_device;
    InputDevice &input_device;
    AudioDevice &audio_device;
    GB_MODEL model;
    bool force_model = false;
    bool save_load_ram = true;
    bool stub_ly = false;
};

} //namespace sb


#endif //GB_COMMON