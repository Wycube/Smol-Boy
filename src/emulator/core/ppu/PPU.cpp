#include "emulator/core/ppu/PPU.hpp"
#include "common/Utility.hpp"

#include <algorithm>


namespace sb {

static const u32 shades[4] = {0xffffffff, 0xb3b3b3ff, 0x6b6b6bff, 0x000000ff};


Fetcher::Fetcher(PPU &ppu) : m_ppu(ppu) { }

Pixel Fetcher::fifo_pop() {
    Pixel pixel = m_bg_fifo.front();
    m_bg_fifo.pop_front();

    //If there is pixels in the sprite fifo, then mix them
    if(!m_sprite_fifo.empty()) {
        Pixel sprite_pixel = m_sprite_fifo.front();
        m_sprite_fifo.pop_front();

        //Priority, only show sprite if background pixel is not color 0
        if(sprite_pixel.priority && pixel.color_index != 0) {
            sprite_pixel = pixel;
        }

        //If sprites palette index is zero, other pixel stays
        if(sprite_pixel.color_index == 0) {
            sprite_pixel = pixel;
        }

        pixel = sprite_pixel;
    }

    m_line_x++;

    return pixel;
}

usize Fetcher::fifo_size() {
    return m_bg_fifo.size();
}

bool Fetcher::disabled() {
    return m_fetch_sprites;
}

void Fetcher::start(std::vector<ObjectData> sprites) {
    u8 y = m_ppu.m_scy + m_ppu.m_ly;
    m_line_x = 0;
    m_tile_index = (m_ppu.m_scx / 8) & 0x1f;
    m_tile_line = y % 8;
    m_map_address = (m_ppu.m_lcdc >> 3) & 1 ? 0x9C00 : 0x9800;
    m_map_address += (y / 8) * 32;
    m_sprites = sprites;
    m_x_scrolled = false;
    m_fetch_sprites = false;
    m_fetch_window = false;
    m_wy_triggered = m_ppu.m_ly >= m_ppu.m_wy;
    m_state = READ_TILE_ID;

    m_bg_fifo.clear();
    m_sprite_fifo.clear();
}

void Fetcher::step() {
    m_ticks++;
    m_tile_address = (m_ppu.m_lcdc >> 4) & 1 ? 0x8000 : 0x9000; //0x9000 uses signed addressing so it can address [0x8800, 0x97FF].
    m_signed_addressing = !((m_ppu.m_lcdc >> 4) & 1);

    //Check for window
    bool window_enabled = (m_ppu.m_lcdc >> 5) & 1;

    if(window_enabled) {
        if(!m_fetch_sprites && !m_fetch_window && (m_ppu.m_wx - 7 <= m_line_x) && m_wy_triggered) {
            //Clear fifo and set stuff for window
            u8 y = m_ppu.m_ly - m_ppu.m_wy;
            m_map_address = (m_ppu.m_lcdc >> 6) & 1 ? 0x9C00 : 0x9800;
            m_map_address += (m_window_y / 8) * 32;
            m_tile_index = 0;
            m_tile_line = m_window_y % 8;
            m_fetch_window = true;
            m_state = READ_TILE_ID;

            m_bg_fifo.clear();
            m_window_y++;
        }
    }

    //Check for sprites at this x
    check_for_obj();

    //Only ticks every other PPU tick
    if(m_ticks < 2 && m_state != PUSH_TO_FIFO) {
        return;
    }    
    m_ticks = 0;

    switch(m_state) {
        case READ_TILE_ID : read_tile_id();
        break;
        case READ_TILE_0 : read_tile_0();
        break;
        case READ_TILE_1 : read_tile_1();
        break;
        case PUSH_TO_FIFO : push_to_fifo();
        break;
    }

    //Check again so there isn't a one pixel offset sometimes
    check_for_obj();
}

void Fetcher::check_for_obj() {
    //Check for sprites at this x
    bool obj_enable = (m_ppu.m_lcdc >> 1) & 1;

    if(obj_enable && !m_fetch_sprites && !m_sprites.empty()) {
        const ObjectData &sprite = m_sprites.back();
        if(sprite.x <= m_line_x + 8) {
            m_sprite_line = (m_ppu.m_ly + 16 - sprite.y);
            m_sprite_width = sprite.x < 8 ? sprite.x : 8; //Clip sprite if it is not entirely in on the left side
            m_current_sprite = sprite;
            m_fetch_sprites = true;
            m_state = READ_TILE_0;

            //Vertical flip
            if((sprite.attribs >> 6) & 1) {
                m_sprite_line = (m_ppu.m_lcdc >> 2) & 1 ? 15 - m_sprite_line : 7 - m_sprite_line;
            }

            m_sprites.pop_back();            
        }
    }
}

void Fetcher::read_tile_id() {
    m_tile_id = m_ppu.read(m_map_address + m_tile_index);
    m_state = READ_TILE_0;
}

void Fetcher::read_tile_0() {
    if(!m_fetch_sprites) {
        //Background stuff
        u16 offset = m_tile_address + (m_signed_addressing ? (s8)m_tile_id * 16 : m_tile_id * 16);
        u16 address = offset + m_tile_line * 2;
        u8 data = m_ppu.read(address);

        for(int i = 0; i < 8; i++) {
            m_pixel_data[i] = (data >> i) & 1;
        }
    } else {
        //Ignore first bit of tile id if it is double height
        u8 sprite_tile_id = (m_ppu.m_lcdc >> 2) & 1 ? m_current_sprite.tile_id & ~1 : m_current_sprite.tile_id;

        u16 offset = 0x8000 + sprite_tile_id * 16;
        u16 address = offset + m_sprite_line * 2;
        u8 data = m_ppu.read(address);

        for(int i = 0; i < 8; i++) {
            m_pixel_data[i] = (data >> i) & 1;
        }
    }

    m_state = READ_TILE_1;
}

void Fetcher::read_tile_1() {
    if(!m_fetch_sprites) {
        //Background stuff
        u16 offset = m_tile_address + (m_signed_addressing ? (s8)m_tile_id * 16 : m_tile_id * 16);
        u16 address = offset + m_tile_line * 2;
        u8 data = m_ppu.read(address + 1);

        for(int i = 0; i < 8; i++) {
            m_pixel_data[i] |= ((data >> i) & 1) << 1;
        }
    } else {
        //Ignore first bit of tile id if it is double height
        u8 sprite_tile_id = (m_ppu.m_lcdc >> 2) & 1 ? m_current_sprite.tile_id & ~1 : m_current_sprite.tile_id;

        u16 offset = 0x8000 + sprite_tile_id * 16;
        u16 address = offset + m_sprite_line * 2;
        u8 data = m_ppu.read(address + 1);

        for(int i = 0; i < 8; i++) {
            m_pixel_data[i] |= ((data >> i) & 1) << 1;
        }
    }

    m_state = PUSH_TO_FIFO;
}

void Fetcher::push_to_fifo() {
    if(m_fetch_sprites) {
        //Push pixels in if not 8 pixels wide
        while(m_sprite_fifo.size() < 8) {
            m_sprite_fifo.push_back(Pixel{0, 1, false});
        }

        //Mix sprites
        for(int i = m_sprite_width - 1; i >= 0; i--) {
            int new_i = i + (8 - m_sprite_width);
            int rev_i = (m_current_sprite.attribs >> 5) & 1 ? new_i : 7 - new_i;
            Pixel pixel = m_sprite_fifo[i];

            if(pixel.color_index == 0 && m_pixel_data[rev_i] != 0) {
                m_sprite_fifo[i] = Pixel{m_pixel_data[rev_i], (u8)(1 + ((m_current_sprite.attribs >> 4) & 1)), ((m_current_sprite.attribs >> 7) & 1) == 1};
            }
        }

        m_fetch_sprites = false;
        m_state = READ_TILE_ID;
    } else if(m_bg_fifo.size() <= 8) {
        for(int i = 7; i >= 0; i--) {
            m_bg_fifo.push_back(Pixel{m_pixel_data[i], 0, false});
        }

        if(!m_x_scrolled && !m_fetch_window) {
            //Only use the lower 3 bits of SCX for "fine scrolling"
            for(int i = 0; i < (m_ppu.m_scx & 7); i++) {
                m_bg_fifo.pop_front();
            }
            m_x_scrolled = true;
        } else if(m_fetch_window && !m_x_scrolled && m_ppu.m_wx < 7) {
            //Scroll based on how much it is offscreen
            for(int i = 0; i < (7 - m_ppu.m_wx); i++) {
                m_bg_fifo.pop_front();
            }
            m_x_scrolled = true;
        }

        m_tile_index = (m_tile_index + 1) % 32;
        m_state = READ_TILE_ID;
    }
}

void Fetcher::vblank() {
    m_window_y = 0;
}


//--------------- PPU ----------------//

PPU::PPU(CPU &cpu, Memory &mem, Clock &clock, VideoDevice &video_device, bool stub_ly) 
: m_cpu(cpu), m_mem(mem), m_clock(clock), m_video_device(video_device), m_stub_ly(stub_ly), m_fetcher(*this) {
    reset();
}

void PPU::reset() {
    m_lcdc = 0xff;
    m_state = HBLANK;
}

//TODO: Blocks certain writes during certain modes
void PPU::write(u16 address, u8 value) {
    u8 old;

    //MSVC doesn't support case ranges
    if(in_range<u16>(address, 0x8000, 0x9FFF)) {
        m_vram[address - 0x8000] = value;
    } else if(in_range<u16>(address, 0xFE00, 0xFE9f)) {
        if(!m_disable_oam) m_oam[address - 0xFE00] = value;
    }
    
    switch(address) {
        // case 0x8000 ... 0x9FFF : m_vram[address - 0x8000] = value;
        // break;
        // case 0xFE00 ... 0xFE9F : if(!m_disable_oam) m_oam[address - 0xFE00] = value;
        // break;

        case 0xFF40 : m_lcdc = value;
        break;
        case 0xFF41 : m_stat = (value & 0b11111000) | (m_stat & 0b00000111); //Bits 0-2 are read-only
            //m_cpu.request_interrupt(LCD_STAT_INT); //Writing to the stat register calls a stat interrupt for some reason
        break;
        case 0xFF42 : m_scy = value;
        break;
        case 0xFF43 : m_scx = value;
        break;
        case 0xFF44 : //LY is read-only
        break;
        case 0xFF45 : m_lyc = value; check_stat_int(); //LY=LYC is checked after a write to LYC
        break;
        case 0xFF46 : m_dma = value; m_dma_start = true;
        break;
        case 0xFF47 : m_bgp = value;
        break;
        case 0xFF48 : m_obp0 = value;
        break;
        case 0xFF49 : m_obp1 = value;
        break;
        case 0xFF4A : m_wy = value;
        break;
        case 0xFF4B : m_wx = value;
        break;
    }
}

u8 PPU::read(u16 address) {
    if(in_range<u16>(address, 0x8000, 0x9FFF)) {
        return m_vram[address - 0x8000];
    } else if(in_range<u16>(address, 0xFE00, 0xFE9f)) {
        if(!m_disable_oam) return m_oam[address - 0xFE00];
        else return 0;
    }

    switch(address) {
        case 0xFF40 : return m_lcdc;
        break;
        case 0xFF41 : return m_stat;
        break;
        case 0xFF42 : return m_scy;
        break;
        case 0xFF43 : return m_scx;
        break;
        case 0xFF44 : if(m_stub_ly) return 0x90; else return m_ly;
        break;
        case 0xFF45 : return m_lyc;
        break;
        case 0xFF46 : return m_dma;
        break;
        case 0xFF47 : return m_bgp;
        break;
        case 0xFF48 : return m_obp0;
        break;
        case 0xFF49 : return m_obp1;
        break;
        case 0xFF4A : return m_wy;
        break;
        case 0xFF4B : return m_wx;
        break;
    }

    return 0;
}

void PPU::step() {
    m_clock.add_t(1);
    m_ticks++;

    bool lcd_enabled = m_lcdc >> 7;

    //LCD disabled
    if(!lcd_enabled) {
        if(!m_disabled) {
            m_ly = 0;
            m_ticks = 0;
            m_state = HBLANK;

            //Clear screen to white
            m_video_device.clear_screen(0xffffffff);
            m_video_device.present_screen();

            m_disabled = true;
            //LOG_INFO("LCD disabled");
        }

        return;
    } else {
        //if(m_disabled) LOG_INFO("LCD Enabled");
        m_disabled = false;
    }

    //I'll just put this here
    update_dma();

    switch(m_state) {
        case OAM_SEARCH : oam_search();
        break;
        case PIXEL_TRANSFER : pixel_transfer();
        break;
        case HBLANK : hblank();
        break;
        case VBLANK : vblank();
        break;
    }

    //Update mode in STAT register
    m_stat = (m_stat & 0xfc) | m_state;
    check_stat_int();
}

void PPU::update_dma() {
    if(m_dma_start) {
        m_mem.dma_start(m_dma);
        m_dma_start = false;
        m_dma_cycles = 0;
    } else if(m_dma_cycles < 160) {
        m_disable_oam = false;
        m_mem.dma_step();
        m_disable_oam = true;
        m_dma_cycles++;
    } else {
        m_disable_oam = false;
    }
}

void PPU::check_stat_int() {
    m_ly_lyc = m_ly == m_lyc;

    //Check for any STAT interrupts
    bool sources[4] = {false};
    sources[0] = m_stat >> 6 & 1;
    sources[1] = m_stat >> 5 & 1;
    sources[2] = m_stat >> 4 & 1;
    sources[3] = m_stat >> 3 & 1;

    //An interrupt only occurs when the STAT line goes from low to high, so these all have to be false, then one can be true
    if((sources[0] && m_ly_lyc) || (sources[1] && m_state == OAM_SEARCH) || (sources[2] && m_state == VBLANK) || (sources[3] && m_state == HBLANK)) {
        if(!m_last_stat_irq) {
            m_cpu.request_interrupt(LCD_STAT_INT);
            m_last_stat_irq = true;
        }
    } else {
        m_last_stat_irq = false;
    }
}

void PPU::oam_search() {
    if(m_ticks == 80) {
        m_lcd_x = 0;
        u8 sprite_height = (m_lcdc >> 2) & 1 ? 16 : 8;
        bool obj_enable = (m_lcdc >> 1) & 1;
        std::vector<ObjectData> sprites;

        //Search OAM for sprites that are on this line
        if(obj_enable) {
            for(u16 i = 0; i <= 0x9F; i += 4) {
                u8 y_pos = m_oam[i];
                u8 x_pos = m_oam[i + 1];

                //Don't add sprites that aren't visible
                if(x_pos == 0 || y_pos == 0) {
                    continue;
                }

                //Check the y to see if it's on this line
                if(y_pos <= m_ly + 16 && m_ly + 16 < y_pos + sprite_height) {
                    sprites.push_back(ObjectData{(u16)(0xFE00 + i), y_pos, x_pos, m_oam[i + 2], m_oam[i + 3]});
                    
                    //No more than 10 sprites a line
                    if(sprites.size() >= 10) {
                        break;
                    }
                }
            }

            //Sort sprites by x and address
            std::sort(sprites.begin(), sprites.end(), [](const ObjectData &first, const ObjectData &second) {
                if(first.x == second.x) {
                    return first.oam_address > second.oam_address;
                } else {
                    return first.x > second.x;
                }
            });
        }

        m_fetcher.start(sprites);
        m_state = PIXEL_TRANSFER;
    }
}

void PPU::pixel_transfer() {
    m_fetcher.step();

    //Don't push pixels while the fifo has less than or equal to 8 pixels or is fetching sprite data
    if(m_fetcher.fifo_size() > 8 && !m_fetcher.disabled()) {
        Pixel pixel = m_fetcher.fifo_pop();
        bool bg_enabled = m_lcdc & 1;

        //Index the background palette
        u8 palette = pixel.palette == 0 ? m_bgp : pixel.palette == 1 ? m_obp0 : m_obp1;
        u8 index = palette >> (pixel.color_index * 2) & 3;
        u32 color = bg_enabled ? shades[index] : pixel.palette != 0 ? shades[index] : shades[0];
        m_video_device.draw_pixel(color, m_lcd_x, m_ly);

        m_lcd_x++;
    }

    if(m_lcd_x == 160) {
        m_state = HBLANK;
    }
}

void PPU::hblank() {
    if(m_ticks == 456) {
        m_ticks = 0;
        m_ly++;
        m_ly_lyc = m_ly == m_lyc;

        //Update lyc=ly in STAT register
        m_stat = m_stat | (m_ly_lyc ? 4 : 0);

        if(m_ly == 144) {
            m_state = VBLANK;
            m_cpu.request_interrupt(VBLANK_INT);
            m_video_device.present_screen();
            m_fetcher.vblank();
        } else {
            m_state = OAM_SEARCH;
        }
    }
}

void PPU::vblank() {
    if(m_ly == 153) {
        m_ly = 0;
        m_ly_lyc = m_ly == m_lyc;
    
        //Update lyc=ly in STAT register
        m_stat = m_stat | (m_ly_lyc ? 4 : 0);
    }

    if(m_ticks == 456) {
        if(m_ly == 0) {
            m_state = OAM_SEARCH;
            m_ticks = 0;
            return;
        }

        m_ticks = 0;
        m_ly++;
        m_ly_lyc = m_ly == m_lyc;

        //Update lyc=ly in STAT register
        m_stat = m_stat | (m_ly_lyc ? 4 : 0);
    }
}

} //namespace sb