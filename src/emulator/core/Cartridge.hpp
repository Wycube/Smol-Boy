#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include "common/Types.hpp"


namespace sb {

struct ROM_Header {
    u8 entry_point[4];        //0x0100 - 0x0103
    u8 nin_logo[48];          //0x0104 - 0x0133

    union {                   //0x0134 - 0x0143
        //DMG header
        u8 dmg_title[16];

        //CGB header
        struct {
            u8 cgb_title[11]; //0x0134 - 0x013E
            u8 manuf_code[4]; //0x013F - 0x0142
            u8 cgb_flag;      //0x0143
        };
    };

    u8 new_licensee[2];       //0x0144 - 0x0145  
    u8 sgb_flag;              //0x0146
    u8 cart_type;             //0x0147
    u8 rom_size;              //0x0148  Refers to amount of ROM banks as (32 KiB << N)
    u8 ram_size;              //0x0149  Refers to amount of RAM banks
    u8 dest_code;             //0x014A
    u8 old_licensee;          //0x014B
    u8 version;               //0x014C
    u8 checksum;              //0x014D
    u8 glob_checksum;         //0x014E - 0x014F
};

struct Cartridge {
private:

    bool m_loaded;

public:

    u8 *rom;
    usize size;
    ROM_Header header;

    Cartridge();
    Cartridge(usize size, u8 *rom);
    ~Cartridge();

    void load(usize size, u8 *rom);
    
    bool loaded();
};

} //namespace sb


#endif //CARTRIDGE_HPP