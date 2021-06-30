#ifndef PPU_HPP
#define PPU_HPP

#include "common/Types.hpp"
#include "emulator/core/Scheduler.hpp"
#include "emulator/core/cpu/CPU.hpp"
#include "emulator/core/Memory.hpp"
#include "emulator/device/VideoDevice.hpp"

#include <deque>
#include <vector>

#define GB_SCREEN_WIDTH 160
#define GB_SCREEN_HEIGHT 144


namespace sb {

//Numbers correspond to numbers in the STAT register
enum PPU_State {
    OAM_SEARCH = 2, PIXEL_TRANSFER = 3, HBLANK = 0, VBLANK = 1
};

enum Fetcher_State {
    READ_TILE_ID, READ_TILE_0, READ_TILE_1, PUSH_TO_FIFO
};

//Data of an OAM entry
struct ObjectData {
    u16 oam_address;
    u8 y;
    u8 x;
    u8 tile_id;
    u8 attribs;

    void operator=(const ObjectData &other) {
        oam_address = other.oam_address;
        y = other.y;
        x = other.x;
        tile_id = other.tile_id;
        attribs = other.attribs;
    }

    bool operator==(ObjectData other) {
        return (oam_address == other.oam_address) && (x == other.x) && (y == other.y) && (tile_id == other.tile_id) && (attribs == other.attribs);
    }
};

//Pixel data
struct Pixel {
    u8 color_index;
    u8 palette;
    bool priority; //Only for sprites

    void operator=(const Pixel &other) {
        color_index = other.color_index;
        palette = other.palette;
        priority = other.priority;
    }
};


class Fetcher {
private:

    //FIFOs
    std::deque<Pixel> m_bg_fifo;
    std::deque<Pixel> m_sprite_fifo;

    //Sprite stuff
    std::vector<ObjectData> m_sprites;
    ObjectData m_current_sprite;
    u8 m_sprite_line;
    u8 m_sprite_width;
    bool m_fetch_sprites;

    Fetcher_State m_state;
    u8 m_ticks;
    u8 m_line_x;
    u8 m_window_y;
    bool m_wy_triggered;
    bool m_fetch_window;

    u8 m_tile_index;
    u8 m_tile_line;
    u8 m_tile_id;
    u16 m_map_address;
    u16 m_tile_address;
    u8 m_pixel_data[8];
    bool m_x_scrolled;
    bool m_signed_addressing;

    PPU &m_ppu;

    void read_tile_id();
    void read_tile_0();
    void read_tile_1();
    void push_to_fifo();

    void check_for_obj();

public:

    Fetcher(PPU &ppu);

    Pixel fifo_pop();
    usize fifo_size();
    bool disabled();
    void start(std::vector<ObjectData> sprites);
    void step();
    void vblank();
};


class PPU {
private:

    //Memory
    u8 m_vram[8192];   //0x8000 - 0x9FFF | Video RAM
    u8 m_oam[160];     //0xFE00 - 0xFE9F | Sprites  

    //IO Registers
    u8 m_lcdc;         //0xFF40 | LCD Ctrl  
    u8 m_stat;         //0xFF41 | LCD Status
    u8 m_scy;          //0xFF42 | Scroll-Y  
    u8 m_scx;          //0xFF43 | Scroll-X  
    u8 m_ly;           //0xFF44 | LCD Y     
    u8 m_lyc;          //0xFF45 | LY Compare
    u8 m_dma;          //0xFF46 | DMA       
    u8 m_bgp;          //0xFF47 | BG Palette
    u8 m_obp0;         //0xFF48 | Palette 0 
    u8 m_obp1;         //0xFF49 | Palette 1 
    u8 m_wy;           //0xFF4A | Window Y  
    u8 m_wx;           //0xFF4B | Window X  

    Fetcher m_fetcher;
    PPU_State m_state;    
    u16 m_ticks;
    u8 m_lcd_x;
    bool m_last_stat_irq;
    bool m_ly_lyc;
    bool m_disabled;
    bool m_dma_start;
    u16 m_dma_cycles;

    bool m_disable_oam;
    bool m_disable_vram;

    CPU &m_cpu;
    Memory &m_mem; //For DMA
    Clock &m_clock;
    VideoDevice &m_video_device;

    //Extra options
    bool m_stub_ly;

    void update_dma();
    void check_stat_int();
    
    void oam_search();
    void pixel_transfer();
    void hblank();
    void vblank();

public:

    PPU(CPU &cpu, Memory &mem, Clock &clock, VideoDevice &video_device, bool stub_ly = false);

    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);
    
    void step();

    friend class Fetcher; //Should probably change this to memory accesses
};

} //namespace sb


#endif //PPU_HPP