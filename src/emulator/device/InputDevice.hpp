#ifndef INPUT_DEVICE_HPP
#define INPUT_DEVICE_HPP

#include "common/Types.hpp"


namespace sb {

class InputDevice {
protected:

public:

    virtual void get_input(u8 &joypad_reg) = 0;
};


class NullInputDevice : public InputDevice {
public:

    void get_input(u8 &joypad_reg) override {
        joypad_reg |= 0xf; //Buttons are always not pressed
    }
};

} //namespace sb


#endif //INPUT_DEVICE_HPP