#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common/Types.hpp"
#include "Cartridge.hpp"
#include "Mapper.hpp"
#include "../device/InputDevice.hpp"

#include <string_view>


namespace sb {

class CPU;
class PPU;
class APU;
class Timer;

class Memory {
private:

    u8 m_boot_rom[256];
    Mapper m_mapper;
                            //ROM bank 00   |  16 Kib  |  0x0000 - 0x3FFF  |  ROM From cartridge
                            //ROM bank NN   |  16 Kib  |  0x4000 - 0x7FFF  |  ROM from cartridge (switchable 01 - NN, used by mapper)
                            //VRAM          |   8 Kib  |  0x8000 - 0x9FFF  |  Video RAM (switchable in CGB Mode, banks 0/1)
                            //External RAM  |   8 Kib  |  0xA000 - 0xBFFF  |  RAM from cartridge (switchable)
    u8 m_iwork_ram[4096];   //Work RAM      |   4 Kib  |  0xC000 - 0xCFFF  |  Internal RAM for general purpose use
    u8 m_ework_ram[4096];   //Work RAM      |   4 Kib  |  0xD000 - 0xDFFF  |  External RAM for general purpose use (switchable in CGB Mode, banks 1-7)
                            //Echo RAM      |  ~7 Kib  |  0xE000 - 0xFDFF  |  A mirror of the RAM at 0xC000 - 0xDDFF, Nintendo says area is prohibited
    u8 m_oam[160];          //OAM           |          |  0xFE00 - 0xFE9F  |  Sprite attribute table, stores stuff about sprites
                            //Unused        |          |  0xFEA0 - 0xFEFF  |  Unused (for some reason), Nintendo says area is prohibited
    u8 m_io_regs[128];      //IO Registers  |          |  0xFF00 - 0xFF7F  |  IO Registers
    u8 m_hram[127];         //High RAM      |          |  0xFF80 - 0xFFFE  |  High RAM
    u8 m_ie;                //IE            |          |  0xFFFF - 0xFFFF  |  Interrupt Enable Register

    Cartridge m_cart;
    CPU &m_cpu;
    PPU &m_ppu;
    APU &m_apu;
    Timer &m_timer;
    InputDevice &m_input_device;

    u16 m_dma_src;
    u16 m_dma_dst;

public:

    Memory(CPU &cpu, PPU &ppu, APU &apu, Timer &timer, InputDevice &input_device);
    ~Memory();

    bool load_cart(const std::string &path);
    bool load_boot(const std::string &path);
    bool loaded();
    Cartridge& get_cart();

    void reset();
    void write(u16 address, u8 value);
    u8 read(u16 address);

    void dma_start(u8 value);
    void dma_step();

    void log_cpu();

    Mapper& get_mapper() { return m_mapper; }
};

} //namespace sb


#endif //MEMORY_HPP