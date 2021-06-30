#ifndef SDL_VIDEO_DEVICE_HPP
#define SDL_VIDEO_DEVICE_HPP

#include "emulator/device/VideoDevice.hpp"

#include <SDL.h>


class SDLVideoDevice : public sb::VideoDevice {
private:

    SDL_Surface *m_surface;
    SDL_Texture *m_texture;

public:

    SDLVideoDevice(u32 width, u32 height) : sb::VideoDevice(width, height), m_texture(nullptr) {
        m_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    }

    ~SDLVideoDevice() {
        SDL_FreeSurface(m_surface);

        if(m_texture != nullptr) {
            SDL_DestroyTexture(m_texture);
        }
    }

    void present_screen() override {
        memcpy(m_surface->pixels, m_internal_buffer, m_surface->w * m_surface->h * 4);
    }

    SDL_Texture* get_texture(SDL_Renderer *renderer) {
        if(m_texture != nullptr) {
            SDL_DestroyTexture(m_texture);
        }

        m_texture = SDL_CreateTextureFromSurface(renderer, m_surface);

        return m_texture;
    }
};


#endif //SDL_VIDEO_DEVICE_HPP