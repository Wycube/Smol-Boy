#ifndef GAMEBOY_HPP
#define GAMEBOY_HPP

#include "cpu/CPU.hpp"
#include "ppu/PPU.hpp"
#include "apu/APU.hpp"
#include "Memory.hpp"
#include "Timer.hpp"
#include "Scheduler.hpp"
#include "GBCommon.hpp"


// 4,194,304 hz / 59.7275 fps
constexpr u32 CYCLES_PER_FRAME = 70244;


namespace sb {

class Gameboy {
private:

    CPU m_cpu;
    PPU m_ppu;
    APU m_apu;
    Memory m_memory;
    Timer m_timer;
    Scheduler m_scheduler;

    GB_MODEL m_model;
    bool m_force_model;
    std::string m_file_name;
    bool m_save_load_ram;

public:

    Gameboy(const std::string &rom_path, const std::string &boot_path, GameboySettings settings);
    ~Gameboy();

    void reset();
    bool rom_loaded();
    bool load_rom(const std::string &rom_path, bool save_load_ram = true);
    bool load_boot(const std::string &path);
    void load_ram();
    void save_ram();

    void run_for(usize cycles);
    std::string get_title();
};

} //namespace sb


#endif //GAMEBOY_HPP