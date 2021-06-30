#ifndef INPUT_DEVICE_HPP
#define INPUT_DEVICE_HPP

#include "common/Types.hpp"

#include <functional>

namespace sb {

using JoypadCallback = std::function<void()>;

class InputDevice {
protected:

    JoypadCallback m_int_callback;

public:

    virtual void get_input(u8 &joypad_reg) = 0;
    void register_callback(JoypadCallback callback) { m_int_callback = callback; }
};


class NullInputDevice : public InputDevice {
public:

    void get_input(u8 &joypad_reg) override {
        joypad_reg |= 0xf; //Buttons are always not pressed
    }
};

} //namespace sb


#endif //INPUT_DEVICE_HPP