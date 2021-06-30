#ifndef SDL_INPUT_DEVICE_HPP
#define SDL_INPUT_DEVICE_HPP

#include "emulator/device/InputDevice.hpp"

#include <SDL.h>


class SDLInputDevice : public sb::InputDevice {
private:

    bool m_action[4], m_con_action[4];
    bool m_dir[4], m_con_dir[4];
    bool m_stick_dir[4];

    bool m_con_connected;
    SDL_GameController *m_con;

public:

    SDLInputDevice() {
        memset(m_action, false, 4);
        memset(m_dir, false, 4);
        memset(m_con_action, false, 4);
        memset(m_con_dir, false, 4);
        memset(m_stick_dir, false, 4);

        m_con_connected = SDL_NumJoysticks();
        m_con = nullptr;
            
        if(m_con_connected) {
            m_con = SDL_GameControllerOpen(0);
            LOG_INFO("Controller Detected: {}", SDL_JoystickNameForIndex(0));
        }
    }

    ~SDLInputDevice() {
        SDL_GameControllerClose(m_con);
    }

    void get_input(u8 &joypad_reg) override {
        bool action_buttons = !((joypad_reg >> 5) & 1);
        bool direct_buttons = !((joypad_reg >> 4) & 1);

        joypad_reg |= 0b11000000;
        joypad_reg &= 0b11110000;

        if(action_buttons) {
            u8 pressed = (!(m_action[0] || m_con_action[0]) << 3) | (!(m_action[1] || m_con_action[1]) << 2) | 
                (!(m_action[2] || m_con_action[2]) << 1) | !(m_action[3] || m_con_action[3]);
            joypad_reg |= pressed;
        } 
        
        if(direct_buttons) {
            u8 pressed = (!(m_dir[0] || m_con_dir[0] || m_stick_dir[0]) << 3) | (!(m_dir[1] || m_con_dir[1] || m_stick_dir[1]) << 2) |
                (!(m_dir[2] || m_con_dir[2] || m_stick_dir[2]) << 1) | !(m_dir[3] || m_con_dir[3] || m_stick_dir[3]);
            joypad_reg |= pressed;
        }

        if(!action_buttons && !direct_buttons) {
            joypad_reg |= 0b1111;
        }
    }

    // A = A, B = S, Select = Z, Start = X, Up = Up Arrow, Right = Right Arrow, Left = Left Arrow, Down = Down Arrow
    void handle_event(SDL_Event &event) {
        if(event.type == SDL_KEYDOWN && !m_con_connected) {
            switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_A : m_action[3] = true;
                break;
                case SDL_SCANCODE_S : m_action[2] = true;
                break;
                case SDL_SCANCODE_Z : m_action[1] = true;
                break;
                case SDL_SCANCODE_X : m_action[0] = true;
                break;
                case SDL_SCANCODE_UP : m_dir[1] = true;
                break;
                case SDL_SCANCODE_RIGHT : m_dir[3] = true;
                break;
                case SDL_SCANCODE_LEFT : m_dir[2] = true;
                break;
                case SDL_SCANCODE_DOWN : m_dir[0] = true;
                default: break;
            }
        } else if(event.type == SDL_KEYUP && !m_con_connected) {
            switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_A : m_action[3] = false;
                break;
                case SDL_SCANCODE_S : m_action[2] = false;
                break;
                case SDL_SCANCODE_Z : m_action[1] = false;
                break;
                case SDL_SCANCODE_X : m_action[0] = false;
                break;
                case SDL_SCANCODE_UP : m_dir[1] = false;
                break;
                case SDL_SCANCODE_RIGHT : m_dir[3] = false;
                break;
                case SDL_SCANCODE_LEFT : m_dir[2] = false;
                break;
                case SDL_SCANCODE_DOWN : m_dir[0] = false;
                default: break;
            }
        }


        //Controller events
        if(event.type == SDL_CONTROLLERBUTTONDOWN) {
            switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A : m_con_action[3] = true;
                break;
                case SDL_CONTROLLER_BUTTON_B : m_con_action[2] = true;
                break;
                case SDL_CONTROLLER_BUTTON_BACK : m_con_action[1] = true;
                break;
                case SDL_CONTROLLER_BUTTON_START : m_con_action[0] = true;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP : m_con_dir[1] = true;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT : m_con_dir[3] = true;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT : m_con_dir[2] = true;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN : m_con_dir[0] = true;
                default: break;
            }
        } else if(event.type == SDL_CONTROLLERBUTTONUP) {
            switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A : m_con_action[3] = false;
                break;
                case SDL_CONTROLLER_BUTTON_B : m_con_action[2] = false;
                break;
                case SDL_CONTROLLER_BUTTON_BACK : m_con_action[1] = false;
                break;
                case SDL_CONTROLLER_BUTTON_START : m_con_action[0] = false;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP : m_con_dir[1] = false;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT : m_con_dir[3] = false;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT : m_con_dir[2] = false;
                break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN : m_con_dir[0] = false;
                default: break;
            }
        }

        if(event.type == SDL_CONTROLLERAXISMOTION) {
            switch(event.caxis.axis) {
                case SDL_CONTROLLER_AXIS_LEFTX : 
                    m_stick_dir[3] = event.caxis.value > 16383;
                    m_stick_dir[2] = event.caxis.value < -16383;
                break;
                case SDL_CONTROLLER_AXIS_LEFTY :
                    m_stick_dir[0] = event.caxis.value > 16383;
                    m_stick_dir[1] = event.caxis.value < -16383;
                break;
            }
        }


        if(event.type == SDL_JOYDEVICEADDED && !m_con_connected) {
            m_con_connected = SDL_NumJoysticks();
            m_con = nullptr;

            if(m_con_connected) {
                m_con = SDL_GameControllerOpen(0);
                LOG_INFO("Controller Detected: {}", SDL_JoystickNameForIndex(0));

                memset(m_action, false, 4);
                memset(m_dir, false, 4);
            }
        } else if(event.type == SDL_JOYDEVICEREMOVED && m_con_connected) {
            m_con_connected = false;
            memset(m_con_action, false, 4);
            memset(m_con_dir, false, 4);
            memset(m_stick_dir, false, 4);
        }
    }
};


#endif //SDL_INPUT_DEVICE_HPP