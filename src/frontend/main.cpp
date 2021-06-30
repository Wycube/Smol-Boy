#include "common/Common.hpp"
#include "emulator/core/Gameboy.hpp"
#include "SDLVideoDevice.hpp"
#include "SDLInputDevice.hpp"
#include "SDLAudioDevice.hpp"
#define ARGPARS_IMPLEMENTATION
#include <argpars.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <SDL.h>

//TODO before going public :
// - Add MBC5


static constexpr struct {
    int MAJOR = 0;
    int MINOR = 3;
    int PATCH = 2;
} VERSION;

static constexpr float GB_SCREEN_RATIO = (float)GB_SCREEN_WIDTH / (float)GB_SCREEN_HEIGHT;

struct ResizeEventInfo {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDLVideoDevice *video_device;
    SDL_Rect *dst_rect;
};

static int resize_event_watch(void *user_data, SDL_Event *event) {
    if(event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        int w, h;
        ResizeEventInfo *info = reinterpret_cast<ResizeEventInfo*>(user_data);
        SDL_GetWindowSize(info->window, &w, &h);

        float ratio = (float)w / (float)h;

        //Calculate rect
        if(ratio >= GB_SCREEN_RATIO) {
            info->dst_rect->w = (float)h * GB_SCREEN_RATIO;
            info->dst_rect->h = h;
            
            info->dst_rect->x = (w - info->dst_rect->w) / 2;
            info->dst_rect->y = 0;
        } else {
            info->dst_rect->w = w;
            info->dst_rect->h = (float)w / GB_SCREEN_RATIO;

            info->dst_rect->x = 0;
            info->dst_rect->y = (h - info->dst_rect->h) / 2;
        }

        //Blit texture onto the screen
        SDL_Texture *texture = info->video_device->get_texture(info->renderer);
        SDL_RenderClear(info->renderer);
        SDL_RenderCopy(info->renderer, texture, 0, info->dst_rect);
        SDL_RenderPresent(info->renderer);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    ap::Options args;
    args.add_option(ap::Builder().lname("help").sname("h").help("Shows this help message.").build());
    args.add_option(ap::Builder().lname("version").sname("v").help("Shows the software version.").build());
    args.add_option(ap::Builder().lname("boot-rom").sname("b").param().def_param("").help("Specifies a path to a 256-byte boot ROM to be loaded.").build());
    args.add_option(ap::Builder().lname("headless").help("Runs the emulator without the window, used for testing and logging.").build());
    args.add_option(ap::Builder().lname("stub-ly").help("Stubs LY to 0x90, or 144. For logging purposes, only used with --headless.").build());
    args.add_option(ap::Builder().lname("no-save").help("Doesn't save MBC external RAM to a file or load from a file.").build());
    args.parse_args(argc, argv);

    //Show usage message
    if(args.is_set_any("help")) {
        fmt::print(args.usage_message(std::string(base_name(argv[0])), "[options...] rom_path"));
        return 0;
    }

    if(args.is_set_any("version")) {
        fmt::print("Copyright (c) 2021 Wycube\nSmol Boy, version {}.{}.{}\n", VERSION.MAJOR, VERSION.MINOR, VERSION.PATCH);
        return 0;
    }

    if(args.other_args.size() == 0) {
        LOG_FATAL("Error: No ROM file provided.");
    }


    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        LOG_FATAL("SDL failed to initialize!");
    }

    //With window
    if(!args.is_set("headless")) {
        //TODO: Embed this later on with some python script version of image-to-array or something
        int w, h, channels;
        u8 *logo_pixels = stbi_load("D:/Spencer/dev/c++/smol-boy/res/logo.png", &w, &h, &channels, 4);
        SDL_Surface *logo = SDL_CreateRGBSurfaceWithFormatFrom((void*)logo_pixels, w, h, channels * sizeof(u8), w * channels, SDL_PIXELFORMAT_RGBA32);

        SDL_Window *window = SDL_CreateWindow("Smol Boy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, GB_SCREEN_WIDTH * 4, GB_SCREEN_HEIGHT * 4, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        SDL_SetWindowIcon(window, logo);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        //Allow dropfile events
        SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

        SDLVideoDevice video_device(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
        SDLInputDevice input_device;
        SDLAudioDevice audio_device;

        SDL_Rect dst_rect = {0, 0, GB_SCREEN_WIDTH * 4, GB_SCREEN_HEIGHT * 4};

        ResizeEventInfo event_info = {window, renderer, &video_device, &dst_rect};
        SDL_AddEventWatch(resize_event_watch, &event_info);

        //Clear screen to white at the start
        video_device.clear_screen(0xffffffff);
        video_device.present_screen();

        sb::Gameboy gb(args.other_args[0], args.get_param_any("boot-rom"), video_device, input_device, audio_device, !args.is_set("no-save"));

        float counter = 0, frame_count = 0, avg_fps = 0;
        bool finished = false;

        while(!finished) {
            u32 start = SDL_GetTicks();
            
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                input_device.handle_event(event);

                if(event.type == SDL_QUIT) {
                    finished = true;
                }

                //Drop file
                if(event.type == SDL_DROPFILE) {
                    gb.save_ram();
                    gb.load_rom(event.drop.file, !args.is_set("no-save"));
                }
            }

            gb.run_for(CYCLES_PER_FRAME);

            SDL_Texture *texture = video_device.get_texture(renderer);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, 0, &dst_rect);
            SDL_RenderPresent(renderer);

            //Get framerate
            frame_count++;
            counter += SDL_GetTicks() - start;

            //Every second
            if(counter >= 1000.0f) {
                avg_fps = frame_count / (counter / 1000.0f);
                frame_count = 0;
                counter = 0;
            }

            SDL_SetWindowTitle(window, fmt::format("Smol Boy - {} | {:.2f} FPS", gb.get_title(), avg_fps).c_str());
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        SDL_FreeSurface(logo);
        stbi_image_free(logo_pixels);
    } else {
        sb::NullVideoDevice video_device(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
        sb::NullInputDevice input_device;
        sb::NullAudioDevice audio_device;
        sb::Gameboy gb(args.other_args[0], args.get_param_any("boot-rom"), video_device, input_device, audio_device, false, args.is_set("stub-ly")); //No saving RAM with headless

        while(true) {
            gb.run_for(CYCLES_PER_FRAME);
        }
    }

    return 0;
}