#include "Cartridge.hpp"
#include "common/Log.hpp"

#include <cstring>
#include <cassert>


namespace sb {

Cartridge::Cartridge() : m_loaded(false), rom(nullptr), size(0) { }

Cartridge::Cartridge(usize rom_size, u8 *rom_data) : m_loaded(true) {
    rom = new u8[rom_size];
    size = rom_size;

    memcpy(rom, rom_data, rom_size);

    //This should probably be changed
    memcpy(header.entry_point, &rom[0x0100], sizeof(ROM_Header::entry_point));
    memcpy(header.nin_logo, &rom[0x0104], sizeof(ROM_Header::nin_logo));
    memcpy(header.dmg_title, &rom[0x0134], sizeof(ROM_Header::dmg_title));
    memcpy(header.new_licensee, &rom[0x0144], sizeof(ROM_Header::new_licensee));
    header.sgb_flag = rom[0x0146];
    header.cart_type = rom[0x0147];
    header.rom_size = rom[0x0148];
    header.ram_size = rom[0x0149];
    header.dest_code = rom[0x014A];
    header.old_licensee = rom[0x014B];
    header.version = rom[0x014C];
    header.checksum = rom[0x014D];
    memcpy(&header.glob_checksum, &rom[0x014C], sizeof(ROM_Header::glob_checksum));
}

Cartridge::~Cartridge() {
    if(rom != nullptr) { 
        delete[] rom;
    }
}

void Cartridge::load(usize rom_size, u8 *rom_data) {
    if(rom != nullptr) { 
        delete[] rom;
    }

    rom = new u8[rom_size];
    size = rom_size;

    memcpy(rom, rom_data, rom_size);

    //This should probably be changed
    memcpy(header.entry_point, &rom[0x0100], sizeof(ROM_Header::entry_point));
    memcpy(header.nin_logo, &rom[0x0104], sizeof(ROM_Header::nin_logo));
    memcpy(header.dmg_title, &rom[0x0134], sizeof(ROM_Header::dmg_title));
    memcpy(header.new_licensee, &rom[0x0144], sizeof(ROM_Header::new_licensee));
    header.sgb_flag = rom[0x0146];
    header.cart_type = rom[0x0147];
    header.rom_size = rom[0x0148];
    header.ram_size = rom[0x0149];
    header.dest_code = rom[0x014A];
    header.old_licensee = rom[0x014B];
    header.version = rom[0x014C];
    header.checksum = rom[0x014D];
    memcpy(&header.glob_checksum, &rom[0x014C], sizeof(ROM_Header::glob_checksum));
    
    m_loaded = true;
}

bool Cartridge::loaded() {
    LOG_INFO("Inquery about the state of the cartridge: Loaded: {}", m_loaded);
    return m_loaded;
}

} //namespace sb