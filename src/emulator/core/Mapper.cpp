#include "Mapper.hpp"
#include "common/Utility.hpp"


namespace sb {

Mapper::Mapper() { }

Mapper::~Mapper() {
    if(m_mbc != nullptr) {
        delete m_mbc;
    }
}

void Mapper::create(u8 code, u8 rom_banks, u8 ram_banks, u8 *rom_data) {
    //Create MBC
    MBCInfo info = info_from_code(code);

    switch(info.type) {
        case NO_MBC : m_mbc = new NoMBC(info.has_ram, info.has_battery); LOG_INFO("[MAP] : No MBC");
        break;
        case MBC_1 : m_mbc = new MBC1(info.has_ram, info.has_battery); LOG_INFO("[MAP] : MBC1");
        break;
        case MBC_3 : m_mbc = new MBC3(info.has_ram, info.has_battery, info.has_timer); LOG_INFO("[MAP] : MBC3");
        break;
    }

    m_mbc->init(rom_banks, ram_banks);
    m_mbc->load_rom(rom_data);
}

bool Mapper::in_address_space(u16 address) {
    return m_mbc->in_address_space(address);
}

void Mapper::write(u16 address, u8 value) {
    m_mbc->write(address, value);
}

u8 Mapper::read(u16 address) {
    return m_mbc->read(address);
}


//--------------- MBC Base Class ---------------//

//All common MBCs work in ROM (0x0000 - 0x7FFF) for reads and register writes, and external RAM (0xA000 - 0xBFFF).
bool MBC::in_address_space(u16 address) {
    return in_range<u16>(address, 0x0000, 0x7FFF) || in_range<u16>(address, 0xA000, 0xBFFF);
}

//For loading RAM from a file
void MBC::load_ram(u8 *ram_data) {
    memcpy(m_ram.data(), ram_data, m_ram_banks * 8 * KiB);
}


//--------------- No MBC ---------------//

//Ignore the number of rom banks provided completely and only add a ram bank if ram_banks is not zero.
void NoMBC::init(u8 rom_banks, u8 ram_banks) {
    m_rom.resize(32 * KiB);
    m_rom_banks = 2;
    m_ram_banks = 0;

    if(ram_banks != 0) {
        m_ram.resize(8 * KiB);
        m_ram_banks = 1;
    }
}

//Load 2 banks of ROM
void NoMBC::load_rom(u8 *rom_data) {
    memcpy(m_rom.data(), rom_data, 32 * KiB);
}

void NoMBC::write(u16 address, u8 value) {
    if(in_range<u16>(address, 0x0000, 0x7FFF)) {
        //ROM - Can't write to ROM on this one
    } else if(in_range<u16>(address, 0xA000, 0xBFFF)) {
        //RAM
        m_ram[address - 0xA000] = value;
    }
}

u8 NoMBC::read(u16 address) {
    if(in_range<u16>(address, 0x0000, 0x7FFF)) {
        //ROM
        return m_rom[address];
    } else if(in_range<u16>(address, 0xA000, 0xBFFF)) {
        //RAM
        return m_ram[address - 0xA000];
    }

    //This should never happen
    return 0;
}


//--------------- MBC1 ---------------//

void MBC1::init(u8 rom_banks, u8 ram_banks) {
    //The second is technically 2Kb of RAM but whatever
    static const u8 ram_bank_sizes[6] = {0, 1, 1, 4, 16, 8};

    m_rom_banks = 2 << rom_banks;
    m_ram_banks = ram_bank_sizes[ram_banks];

    LOG_INFO("[MBC1] : Number of ROM banks: {}", m_rom_banks);
    LOG_INFO("[MBC1] : Number of RAM banks: {}", m_ram_banks);

    m_rom.resize(m_rom_banks * 16 * KiB);
    m_ram.resize(m_ram_banks * 8 * KiB);
}

void MBC1::load_rom(u8 *rom_data) {
    memcpy(m_rom.data(), rom_data, m_rom_banks * 16 * KiB);
}

void MBC1::write(u16 address, u8 value) {
    if(in_range<u16>(address, 0x0000, 0x1FFF)) {
        //RAM Enable
        m_ram_enable = (value & 0xf) == 0xa;
    } else if(in_range<u16>(address, 0x2000, 0x3FFF)) {
        //ROM Bank Number, only affects lower 5 bits
        if((value & 0x1f) == 0) value = 1; //MBC1 bug, 0x20, 0x40, 0x60 become 0x21, 0x41, 0x61 
        
        m_selected_bank = value & 0x1f;
    } else if(in_range<u16>(address, 0x4000, 0x5FFF)) {
        //RAM Bank Number or Upper Bits of ROM Bank Number
        m_selected_bank2 = value & 3;
    } else if(in_range<u16>(address, 0x6000, 0x7FFF)) {
        //ROM/RAM Mode Select
        m_mode = value & 1;
    } else if(in_range<u16>(address, 0xA000, 0xBFFF) && m_ram_banks != 0) {
        //RAM bank whatever
        u8 ram_bank = m_mode ? m_selected_bank2 : 0;
        if(m_ram_enable) m_ram[(address - 0xA000) + ram_bank * 8 * KiB] = value;
    }
}

u8 MBC1::read(u16 address) {
    if(in_range<u16>(address, 0x0000, 0x3FFF)) {
        //ROM bank 0
        u8 rom_bank = m_mode ? m_selected_bank2 << 5 : 0;

        u32 new_address = (address & 0x3FFF) | (rom_bank * 16 * KiB);

        //return m_rom[address + rom_bank * 16 * kilobyte];
        return m_rom[new_address & (m_rom.size() - 1)];
    } else if(in_range<u16>(address, 0x4000, 0x7FFF)) {
        //ROM bank whatever
        //u8 rom_bank = m_mode ? m_selected_bank : m_selected_bank | (m_selected_bank2 << 5);
        u8 rom_bank = m_selected_bank | (m_selected_bank2 << 5);

        u32 new_address = (address & 0x3FFF) | (rom_bank * 16 * KiB);

        //return m_rom[(address - 0x4000) + rom_bank * 16 * kilobyte];
        return m_rom[new_address & (m_rom.size() - 1)];
    } else if(in_range<u16>(address, 0xA000, 0xBFFF) && m_ram_banks != 0) {
        //RAM bank whatever
        u8 ram_bank = m_mode ? m_selected_bank2 : 0;
        u32 new_address = (address & 0x1FFF) | (ram_bank * 8 * KiB);
        //if(m_ram_enable) return m_ram[(address - 0xA000) + ram_bank * 8 * kilobyte];
        if(m_ram_enable) return m_ram[new_address & (m_ram.size() - 1)];
    }

    return 0xff;
}


//--------------- MBC3 ---------------//

void MBC3::init(u8 rom_banks, u8 ram_banks) {
    //The second is technically 2Kib of RAM but whatever
    static const u8 ram_bank_sizes[6] = {0, 1, 1, 4, 16, 8};

    m_rom_banks = 2 << rom_banks;
    m_ram_banks = ram_bank_sizes[ram_banks];

    LOG_INFO("[MBC3] : Number of ROM banks: {}", m_rom_banks);
    LOG_INFO("[MBC3] : Number of RAM banks: {}", m_ram_banks);

    m_rom.resize(m_rom_banks * 16 * KiB);
    m_ram.resize(m_ram_banks * 8 * KiB);
}

void MBC3::load_rom(u8 *rom_data) {
    memcpy(m_rom.data(), rom_data, m_rom_banks * 16 * KiB);
}

void MBC3::write(u16 address, u8 value) {
    if(in_range<u16>(address, 0x0000, 0x1FFF)) {
        //RAM Enable
        m_ram_enable = (value & 0xf) == 0xa;
    } else if(in_range<u16>(address, 0x2000, 0x3FFF)) {
        //ROM Bank Number, 7 bits, for 128 ROM banks
        if((value & 0b1111111) == 0) value = 1;

        m_selected_rom = value & 0x7f;
    } else if(in_range<u16>(address, 0x4000, 0x5FFF)) {
        //RAM Bank Number or RTC Register
        m_selected_ram = value; //RAM banks is 00-07, RTC is 08-0C
    } else if(in_range<u16>(address, 0x6000, 0x7FFF)) {
        //Latch clock data
    } else if(in_range<u16>(address, 0xA000, 0xBFFF) && m_ram_banks != 0 && m_selected_ram < 8) {
        //RAM bank whatever
        if(m_ram_enable) m_ram[(address - 0xA000) + m_selected_ram * 8 * KiB] = value;
    }
}

u8 MBC3::read(u16 address) {
    if(in_range<u16>(address, 0x0000, 0x3FFF)) {
        //ROM bank 0
        return m_rom[address];
    } else if(in_range<u16>(address, 0x4000, 0x7FFF)) {
        //ROM bank whatever
        return m_rom[(address - 0x4000) + m_selected_rom * 16 * KiB];
    } else if(in_range<u16>(address, 0xA000, 0xBFFF) && m_ram_banks != 0 && m_selected_ram < 8) {
        //RAM bank whatever
        if(m_ram_enable) return m_ram[(address - 0xA000) + m_selected_ram * 8 * KiB];
    } else if(in_range<u16>(address, 0xA000, 0xBFFF)){
        //LOG_INFO("Attempting to read from RTC");
    }

    return 0xff;
}

} //namespace sb