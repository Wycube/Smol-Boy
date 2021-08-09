#ifndef SDL_VIDEO_DEVICE_HPP
#define SDL_VIDEO_DEVICE_HPP

#include "emulator/device/VideoDevice.hpp"

#include <SDL.h>


class SDLVideoDevice : public sb::VideoDevice {
private:

    SDL_Surface *m_surface;
    SDL_Renderer *m_renderer;
    SDL_Texture *m_texture;

public:

    SDLVideoDevice(SDL_Window *window, u32 width, u32 height) : sb::VideoDevice(width, height), m_texture(nullptr) {
        m_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
        m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);// | SDL_RENDERER_PRESENTVSYNC);
    }

    ~SDLVideoDevice() {
        SDL_FreeSurface(m_surface);

        if(m_texture != nullptr) {
            SDL_DestroyTexture(m_texture);
        }

        SDL_DestroyRenderer(m_renderer);
    }

    void present_screen() override {
        memcpy(m_surface->pixels, m_internal_buffer, m_surface->w * m_surface->h * 4);
    }

    void present_to_window(SDL_Rect dst_rect) {
        SDL_Texture *texture = get_texture();
        SDL_RenderClear(m_renderer);
        SDL_RenderCopy(m_renderer, texture, 0, &dst_rect);
        SDL_RenderPresent(m_renderer);
    }

    SDL_Texture* get_texture() {
        if(m_texture != nullptr) {
            SDL_DestroyTexture(m_texture);
        }

        m_texture = SDL_CreateTextureFromSurface(m_renderer, m_surface);

        return m_texture;
    }

    SDL_Renderer* get_renderer() {
        return m_renderer;
    }
};


#endif //SDL_VIDEO_DEVICE_HPP