#include "common/Common.hpp"
#include "emulator/core/Gameboy.hpp"
#include "SDLVideoDevice.hpp"
#include "SDLInputDevice.hpp"
#include "SDLAudioDevice.hpp"
#define ARGPARS_IMPLEMENTATION
#include <argpars.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <SDL.h>
#include <iostream>


static constexpr struct {
    int MAJOR = 0;
    int MINOR = 3;
    int PATCH = 3;
} VERSION;

static constexpr float GB_SCREEN_RATIO = (float)GB_SCREEN_WIDTH / (float)GB_SCREEN_HEIGHT;

struct ResizeEventInfo {
    SDL_Window *window;
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
        info->video_device->present_to_window(*info->dst_rect);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    ap::Options args;
    args.add_option(ap::Builder().lname("help").sname("h").help("Shows this help message.").build());
    args.add_option(ap::Builder().lname("version").sname("v").help("Shows the software version.").build());
    args.add_option(ap::Builder().lname("boot-rom").sname("b").param().help("Specifies a path to a 256-byte boot ROM to be loaded.").build());
    args.add_option(ap::Builder().lname("headless").help("Runs the emulator without the window, used for testing and logging.").build());
    args.add_option(ap::Builder().lname("stub-ly").help("Stubs LY to 0x90, or 144. For logging purposes, only used with --headless.").build());
    args.add_option(ap::Builder().lname("no-save").help("Doesn't save MBC external RAM to a file or load from a file.").build());
    args.add_option(ap::Builder().lname("force-model").sname("f").param().def_param("DMG").help("Forces a certain Gameboy model (DMG or CGB).").build());
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

    //Check if forced-model is valid one
    sb::GB_MODEL model = sb::DMG;

    if(args.is_set_any("f")) {
        if(args.get_param_any("f") == "DMG") {
            model = sb::DMG;
        } else if(args.get_param_any("f") == "CGB") {
            model = sb::CGB;
        } else {
            LOG_FATAL("Invalid model {} provided!", args.get_param_any("f"));
        }

        LOG_INFO("Forcing model {}", args.get_param_any("f"));
    }

    //With window
    if(!args.is_set("headless")) {
        int w, h, channels;
        u8 *logo_pixels = stbi_load("logo.png", &w, &h, &channels, 4);
        SDL_Surface *logo = SDL_CreateRGBSurfaceWithFormatFrom((void*)logo_pixels, w, h, channels * sizeof(u8), w * channels, SDL_PIXELFORMAT_RGBA32);

        SDL_Window *window = SDL_CreateWindow("Smol Boy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, GB_SCREEN_WIDTH * 4, GB_SCREEN_HEIGHT * 4, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        SDL_SetWindowIcon(window, logo);

        //Allow dropfile events
        SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

        SDLVideoDevice video_device(window, GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
        SDLInputDevice input_device;
        SDLAudioDevice audio_device;

        sb::Gameboy gb(args.other_args[0], args.get_param_any("boot-rom"), {video_device, input_device, audio_device, model, args.is_set_any("f"), !args.is_set("no-save")});
        audio_device.set_sync(true, &gb);
        audio_device.start();

        SDL_SetWindowTitle(window, fmt::format("Smol Boy - {}", gb.get_title()).c_str());

        SDL_Rect dst_rect = {0, 0, GB_SCREEN_WIDTH * 4, GB_SCREEN_HEIGHT * 4};

        ResizeEventInfo event_info = {window, &video_device, &dst_rect};
        SDL_AddEventWatch(resize_event_watch, &event_info);

        //Clear screen to white at the start
        video_device.clear_screen(0xffffffff);
        video_device.present_screen();

        bool finished = false;

        while(!finished) {
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
                    SDL_SetWindowTitle(window, fmt::format("Smol Boy - {}", gb.get_title()).c_str());
                }

                //Fast Forward Hotkey
                if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_U) {
                    audio_device.stop();
                    audio_device.set_sync(!audio_device.syncing(), &gb);
                    audio_device.start();
                }
            }

            if(!audio_device.syncing()) {
                gb.run_for(CYCLES_PER_FRAME);
            }

            video_device.present_to_window(dst_rect);
        }

        SDL_DestroyWindow(window);

        SDL_FreeSurface(logo);
        stbi_image_free(logo_pixels);
    } else {
        sb::NullVideoDevice video_device(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
        sb::NullInputDevice input_device;
        sb::NullAudioDevice audio_device;
        sb::Gameboy gb(args.other_args[0], args.get_param_any("boot-rom"), {video_device, input_device, audio_device, model, args.is_set_any("f"), false, args.is_set("stub-ly")}); //No saving RAM with headless

        while(true) {
            gb.run_for(CYCLES_PER_FRAME);
        }
    }

    return 0;
}
