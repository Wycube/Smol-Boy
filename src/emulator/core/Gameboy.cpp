#include "Gameboy.hpp"
#include "common/Log.hpp"

#include <filesystem>


namespace sb {

Gameboy::Gameboy(const std::string &rom_path, const std::string &boot_path, VideoDevice &video_device, InputDevice &input_device, AudioDevice &audio_device, bool save_load_ram, bool stub_ly)
: m_memory(m_cpu, m_ppu, m_apu, m_timer, input_device), m_cpu(m_memory, m_scheduler.cpu_clock, boot_path.empty()), 
m_ppu(m_cpu, m_memory, m_scheduler.ppu_clock, video_device, stub_ly), m_apu(m_timer, audio_device), m_timer(m_cpu) {
    m_scheduler.set_cpu_step([&]() {
        if(!m_cpu.halted() && !m_cpu.stopped()) {
            m_cpu.step();
        } else {
            //Execute a nop while halted
            m_cpu.nop();
        }
        m_cpu.service_interrupts(); 
    });
    m_scheduler.set_ppu_step([&]() { 
        if(!m_cpu.stopped()) {
            m_ppu.step(); m_apu.step(); m_timer.step();
        } else {
            m_ppu.cycle_empty(); //Keep the clock ticking
        }
    });

    load_rom(rom_path, save_load_ram);
    if(!boot_path.empty()) load_boot(boot_path);
}

Gameboy::~Gameboy() {
    if(m_save_load_ram) {
        save_ram();
    }
}

void Gameboy::reset() {
    m_scheduler.reset();
    m_memory.reset();
    m_timer.reset();
    m_cpu.reset();
    m_ppu.reset();
    m_apu.reset();
}

bool Gameboy::load_rom(const std::string &rom_path, bool save_load_ram) {
    m_save_load_ram = save_load_ram;
    m_file_name = file_name(rom_path);

    reset();

    if(!m_memory.load_cart(rom_path)) {
        return false;
    }

    if(save_load_ram) {
        load_ram();
    }

    return true;
}

bool Gameboy::load_boot(const std::string &path) {
    return m_memory.load_boot(path);
}

void Gameboy::load_ram() {
    MBC *mbc = m_memory.get_mapper().get_mbc();
    
    if(mbc->num_ram_banks() == 0 || !mbc->has_battery()) {
        return;
    }

    //Make sure it exists
    if(!std::filesystem::exists(m_file_name + ".ram")) {
        return;
    }

    usize save_size = std::filesystem::file_size(m_file_name + ".ram");

    if(save_size != mbc->num_ram_banks() * 8 * KiB) {
        LOG_FATAL("Save file not size needed: Expected {} Kib", mbc->num_ram_banks() * 8);
    }

    std::ifstream save_file(m_file_name + ".ram", std::ios_base::binary | std::ios_base::in);

    u8 *ram = new u8[save_size];
    save_file.read((char*)ram, save_size);
    mbc->load_ram(ram);

    delete[] ram;

    LOG_INFO("Loaded RAM data from {}", base_name(m_file_name + ".ram"));
}

void Gameboy::save_ram() {
    MBC *mbc = m_memory.get_mapper().get_mbc();
    
    if(mbc->num_ram_banks() == 0 || !mbc->has_battery()) {
        return;
    }

    std::ofstream save_file(m_file_name + ".ram", std::ios_base::binary | std::ios_base::out);

    if(!save_file.good()) {
        LOG_FATAL("Failed to save RAM to {}", m_file_name + ".ram");
    }

    for(usize i = 0; i < mbc->get_ram_banks().capacity(); i++) {
        save_file << mbc->get_ram_banks()[i];
    }

    LOG_INFO("Saved RAM data to {}", base_name(m_file_name + ".ram"));
}

void Gameboy::run_for(usize cycles) {
    m_scheduler.run_for(cycles);
}

std::string Gameboy::get_title() {
    return std::string((char*)m_memory.get_cart().header.dmg_title);
}

} //namespace sb