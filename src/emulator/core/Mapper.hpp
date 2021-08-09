#ifndef MAPPER_HPP
#define MAPPER_HPP

#include "common/Types.hpp"
#include "common/Log.hpp"

#include <vector>
#include <unordered_map>


namespace sb {

enum MBC_Type {
    NO_MBC, MBC_1, MBC_3, MBC_5
};

struct MBCInfo {
    MBC_Type type;
    bool has_ram;
    bool has_battery;
    bool has_timer = false;
};

constexpr MBCInfo info_from_code(u8 code) {
    switch(code) {
        case 0x00 : return {NO_MBC, false, false};
        case 0x01 : return {MBC_1, false, false};
        case 0x02 : return {MBC_1, true, false};
        case 0x03 : return {MBC_1, true, true};
        case 0x08 : return {NO_MBC, true, false};
        case 0x0f : return {MBC_3, false, true, true};
        case 0x10 : return {MBC_3, true, true, true};
        case 0x11 : return {MBC_3, false, false};
        case 0x12 : return {MBC_3, true, false};
        case 0x13 : return {MBC_3, true, true};
        case 0x19 : return {MBC_5, false, false};
        case 0x1A : return {MBC_5, true, false};
        case 0x1B : return {MBC_5, true, true};
        //MBC5's with rumble

        default : LOG_FATAL("Mapper of type 0x{:02X} is not yet supported!", code);
    }
}

//MBC (Memory Bank Controller), intercepts writes to certain ROM or RAM addresses on the cartridge and switches ROM or RAM banks
//allowing for much more memory than a plain Gameboy cartridge can hold.
class MBC {
protected:

    std::vector<u8> m_rom;
    std::vector<u8> m_ram;
    u8 m_rom_banks;
    u8 m_ram_banks;

    bool m_has_ram;
    bool m_has_battery;

public:

    MBC(bool has_ram, bool has_battery) : m_has_ram(has_ram), m_has_battery(has_battery) { }
    virtual ~MBC() { }

    bool in_address_space(u16 address);

    virtual void init(u8 rom_banks, u8 ram_banks) = 0;
    virtual void load_rom(u8 *rom_data) = 0;
    virtual void write(u16 address, u8 value) = 0;
    virtual u8 read(u16 address) = 0;
    void load_ram(u8 *ram_data);

    bool has_ram() { return m_has_ram; }
    bool has_battery() { return m_has_battery; }

    u8 num_ram_banks() { return m_ram_banks; }
    std::vector<u8>& get_ram_banks() { return m_ram; }
};


class Mapper {
private:

    MBC *m_mbc;

public:

    Mapper();
    ~Mapper();

    void create(u8 code, u8 rom_banks, u8 ram_banks, u8 *rom_data);
    bool in_address_space(u16 address);
    void write(u16 address, u8 value);
    u8 read(u16 address);

    MBC* get_mbc() { return m_mbc; }
};


//Behaves exactly as if there was no MBC at all. Only 2 banks of ROM, 32Kb, and optionally 1 bank of work RAM.
class NoMBC : public MBC {
public:

    NoMBC(bool has_ram, bool has_battery) : MBC(has_ram, has_battery) { }

    void init(u8 rom_banks, u8 ram_banks) override;
    void load_rom(u8 *rom_data) override;
    void write(u16 address, u8 value) override;
    u8 read(u16 address) override;
};


//Has switchable ROM and RAM banks but it can only use up to 125 because of some bug.
class MBC1 : public MBC {
private:

    u8 m_selected_bank;  //Bank1 register
    u8 m_selected_bank2; //Bank2 register
    u8 m_mode;
    bool m_ram_enable;

public:

    MBC1(bool has_ram, bool has_battery) : MBC(has_ram, has_battery), m_ram_enable(false), m_selected_bank(1), m_selected_bank2(0), m_mode(0) { }

    void init(u8 rom_banks, u8 ram_banks) override;
    void load_rom(u8 *rom_data) override;
    void write(u16 address, u8 value) override;
    u8 read(u16 address) override;
};


//Has switchable ROM and RAM banks, has fixed the bug in MBC1, and has a RTC (Real Time Clock)
class MBC3 : public MBC {
private:

    u8 m_selected_rom;
    u8 m_selected_ram;
    bool m_ram_enable;

    bool m_has_timer;

public:

    MBC3(bool has_ram, bool has_battery, bool has_timer) : MBC(has_ram, has_battery), m_has_timer(has_timer), m_ram_enable(true), m_selected_rom(1), m_selected_ram(0) { }

    void init(u8 rom_banks, u8 ram_banks) override;
    void load_rom(u8 *rom_data) override;
    void write(u16 address, u8 value) override;
    u8 read(u16 address) override;

    bool has_timer() { return m_has_timer; }
};


class MBC5 : public MBC {
private:

    u16 m_selected_rom;
    u8 m_selected_ram;
    bool m_ram_enable;

public:

    MBC5(bool has_ram, bool has_battery) : MBC(has_ram, has_battery), m_ram_enable(true), m_selected_rom(1), m_selected_ram(0) { }

    void init(u8 rom_banks, u8 ram_banks) override;
    void load_rom(u8 *rom_data) override;
    void write(u16 address, u8 value) override;
    u8 read(u16 address) override;
};

} //namespace sb


#endif //MAPPER_HPP