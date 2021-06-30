#include "Memory.hpp"
#include "common/Log.hpp"
#include "cpu/CPU.hpp"
#include "ppu/PPU.hpp"
#include "apu/APU.hpp"
#include "Timer.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>

namespace sb {

Memory::Memory(CPU &cpu, PPU &ppu, APU &apu, Timer &timer, InputDevice &input_device) : m_cpu(cpu), m_ppu(ppu), m_apu(apu), m_timer(timer), m_input_device(input_device) {
    reset();
}

Memory::~Memory() { }

bool Memory::load_cart(const std::string &path) {
    if(!std::filesystem::exists(path)) {
        LOG_FATAL("File does not exist!");
        return false;
    }
    
    usize size = std::filesystem::file_size(path);
    LOG_INFO("size = {}", size);

    //Max GB ROM size 8 MB
    if(size > 8000 * KiB) {
        LOG_FATAL("File too large!");
        return false;
    }

    std::fstream file(path, std::ifstream::in | std::ios::binary);

    if(!file.good()) {
        LOG_FATAL("File not good! Bad: {}, Fail {}", file.bad(), file.fail());
        return false;
    }

    u8 *rom = new u8[size];
    file.read((char*)rom, size);
    m_cart.load(size, rom);

    m_mapper.create(m_cart.header.cart_type, m_cart.header.rom_size, m_cart.header.ram_size, rom);

    delete[] rom;

    return true;
}

bool Memory::load_boot(const std::string &path) {
    if(!std::filesystem::exists(path)) {
        LOG_FATAL("Boot ROM File does not exist!");
        return false;
    }
    
    usize size = std::filesystem::file_size(path);

    if(size > 256) {
        LOG_FATAL("Boot ROM File too large!");
        return false;
    }

    std::fstream file(path, std::ifstream::in | std::ios::binary);

    if(!file.good()) {
        LOG_FATAL("Boot ROM File not good! Bad: {}, Fail {}", file.bad(), file.fail());
        return false;
    }

    file.read((char*)m_boot_rom, size);

    return true;
}

bool Memory::loaded() {
    return m_cart.loaded();
}

Cartridge& Memory::get_cart() {
    return m_cart;
}

void Memory::reset() {
    m_ie = 0xff;
    memset(m_io_regs, 0, 128);
}

void Memory::write(u16 address, u8 value) {
    //Do a binary search sort of thing
    if(address <= 0xFDFF) {
        if(address <= 0x7FFF) {
            //ROM
            m_mapper.write(address, value);
        } else {
            u8 top_nibble = address >> 12;

            if(in_range<u8>(top_nibble, 0x8, 0x9)) {
                //VRAM
                return m_ppu.write(address, value);
            } else if(in_range<u8>(top_nibble, 0xA, 0xB)) {
                //External RAM
                return m_mapper.write(address, value);
            } else if(in_range<u8>(top_nibble, 0xC, 0xC)) {
                //Internal Work RAM
                m_iwork_ram[address - 0xC000] = value;
            } else if(in_range<u8>(top_nibble, 0xD, 0xD)) {
                //External Work RAM
                m_ework_ram[address - 0xD000] = value;
            } else {
                //Echo RAM
                if(top_nibble == 0xE) {
                    m_iwork_ram[address - 0xE000] = value;
                } else {
                    m_ework_ram[address - 0xF000] = value;
                }
            }
        }
    } else {
        u8 bottom_byte = address & 0xff;

        if(address <= 0xFEFF) {
            if(in_range<u8>(bottom_byte, 0x00, 0x9F)) {
                //OAM
                m_ppu.write(address, value);
            }
        } else {
            if(in_range<u8>(bottom_byte, 0x00, 0x7F)) {
                //IO Registers
                if(in_range<u8>(bottom_byte, 0x04, 0x07)) {
                    //Timers
                    m_timer.write(address, value);
                } else if(in_range<u8>(bottom_byte, 0x10, 0x3F)) {
                    //Sound
                    m_apu.write(address, value);
                } else if(in_range<u8>(bottom_byte, 0x40, 0x4B)) {
                    //LCD
                    m_ppu.write(address, value);
                } else {
                    //Everything else
                    m_io_regs[address - 0xFF00] = value;
                }
            } else if(in_range<u8>(bottom_byte, 0x80, 0xFE)) {
                //High RAM
                m_hram[address - 0xFF80] = value;
            } else {
                //IE
                m_ie = value;
            }
        }
    }

    if(address == 0xFF02 && value == 0x81) {
        char c = m_io_regs[1];
        std::printf("%c", c);
    }
}

u8 Memory::read(u16 address) {
    //Do a binary search sort of thing
    if(address <= 0xFDFF) {
        if(address <= 0x7FFF) {
            if(m_io_regs[0x50] == 0 && in_range<u16>(address, 0x0000, 0x0100)) {
                //Boot ROM
                return m_boot_rom[address];
            }
            
            //ROM
            return m_mapper.read(address);
        } else {
            u8 top_nibble = address >> 12;

            if(in_range<u8>(top_nibble, 0x8, 0x9)) {
                //VRAM
                return m_ppu.read(address);
            } else if(in_range<u8>(top_nibble, 0xA, 0xB)) {
                //External RAM
                return m_mapper.read(address);
            } else if(in_range<u8>(top_nibble, 0xC, 0xC)) {
                //Internal Work RAM
                return m_iwork_ram[address - 0xC000];
            } else if(in_range<u8>(top_nibble, 0xD, 0xD)) {
                //External Work RAM
                return m_ework_ram[address - 0xD000];
            } else {
                //Echo RAM
                if(top_nibble == 0xE) {
                    return m_iwork_ram[address - 0xE000];
                } else {
                    return m_ework_ram[address - 0xF000];
                }
            }
        }
    } else {
        u8 bottom_byte = address & 0xff;

        if(address <= 0xFEFF) {
            if(in_range<u8>(bottom_byte, 0x00, 0x9F)) {
                //OAM
                return m_ppu.read(address);
            } else {
                //Unused
                return 0xff;
            }
        } else {
            if(in_range<u8>(bottom_byte, 0x00, 0x7F)) {
                //IO Registers
                if(bottom_byte == 0) {
                    //Controller
                    u8 old = m_io_regs[0];
                    m_input_device.get_input(m_io_regs[0]);

                    //Check if a button was pressed, buttons go low when pressed so the number should be less
                    if(old > m_io_regs[0]) {
                        m_cpu.request_interrupt(JOYPAD_INT);
                    }

                    return m_io_regs[0];
                } else if(in_range<u8>(bottom_byte, 0x04, 0x07)) {
                    //Timers
                    return m_timer.read(address);
                } else if(in_range<u8>(bottom_byte, 0x10, 0x3F)) {
                    //Sound
                    return m_apu.read(address);
                } else if(in_range<u8>(bottom_byte, 0x40, 0x4B)) {
                    //LCD
                    return m_ppu.read(address);
                } else {
                    //Everything else
                    return m_io_regs[address - 0xFF00];
                }
            } else if(in_range<u8>(bottom_byte, 0x80, 0xFE)) {
                //High RAM
                return m_hram[address - 0xFF80];
            } else {
                //IE
                return m_ie;
            }
        }
    }

    return 0;
}

void Memory::dma_start(u8 value) {
    m_dma_src = value << 8;
    m_dma_dst = 0xFE00;
}

//Not sure this is entirely accurate, supposedly it takes more than 160 cycles
void Memory::dma_step() {
    m_ppu.write(m_dma_dst++, read(m_dma_src++));
}

} //namespace sb