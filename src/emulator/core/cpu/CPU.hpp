#ifndef CPU_HPP
#define CPU_HPP

#include "common/Defines.hpp"
#include "common/Types.hpp"
#include "common/Log.hpp"
#include "common/Utility.hpp"
#include "emulator/core/Memory.hpp"
#include "emulator/core/Scheduler.hpp"
#include "emulator/core/GBCommon.hpp"
#include "mnemonic.hpp"

#include <array>
#include <fstream>

namespace sb {

enum Reg8 : u8 {
    A, F, B, C, D, E, H, L
};

enum Reg16 : u8 {
    AF, BC, DE, HL, SP, PC
};

union Reg {
    struct {
        #if SB_BYTE_ORDER == SB_LITTLE_ENDIAN
        u8 lo;
        u8 hi;
        #elif SB_BYTE_ORDER == SB_BIG_ENDIAN
        u8 hi;
        u8 lo;
        #endif
    };

    u16 value;
};

enum Flag : u8 {
    ZERO = 128, SUBTRACTION = 64, HALF_CARRY = 32, CARRY = 16
};

enum ALU_Op {
    ADD, ADC, SUB, SBC, AND, XOR, OR, CP
};

enum Interrupt : u8 {
    VBLANK_INT = 1, LCD_STAT_INT = 2, TIMER_INT = 4, SERIAL_INT = 8, JOYPAD_INT = 16
};

//A Sharp SM83 or Sharp LR35902 implementation
class CPU {
private:

    //Instruction LUTs

    u8 illegal(u8 first, u8 second);
    u8 unknown(u8 first, u8 second);
    void cb_unknown();

    u8 nop(u8 first, u8 second);
    u8 halt(u8 first, u8 second);
    u8 stop(u8 first, u8 second);

    template<Reg8 dest, Reg8 src>
    u8 ld_r_r(u8 first, u8 second);
    template<Reg8 dest>
    u8 ld_r_hl(u8 first, u8 second);
    template<Reg8 src>
    u8 ld_hl_r(u8 first, u8 second);
    template<Reg8 dest>
    u8 ld_r_n(u8 first, u8 second);
    u8 ld_hl_n(u8 first, u8 second);
    template<Reg16 src_indirect>
    u8 ld_a_ri(u8 first, u8 second);
    template<Reg16 dest_indirect>
    u8 ld_ri_a(u8 first, u8 second);
    template<s8 adder>
    u8 ld_hli_a(u8 first, u8 second);
    template<s8 adder>
    u8 ld_a_hli(u8 first, u8 second);
    template<Reg16 dest>
    u8 ld_rr_nn(u8 first, u8 second);
    u8 ld_nn_sp(u8 first, u8 second);
    u8 read_io_n(u8 first, u8 second);
    u8 write_io_n(u8 first, u8 second);
    u8 read_io_c(u8 first, u8 second);
    u8 write_io_c(u8 first, u8 second);
    u8 ld_nn_a(u8 first, u8 second);
    u8 ld_a_nn(u8 first, u8 second);
    u8 ld_sp_hl(u8 first, u8 second);
    u8 ld_hl_spn(u8 first, u8 second);
    
    u8 abs_jp(u8 first, u8 second);
    template<Flag if_set>
    u8 abs_jp_if(u8 first, u8 second);
    template<Flag not_set>
    u8 abs_jp_if_not(u8 first, u8 second);
    u8 abs_jp_hl(u8 first, u8 second);
    u8 rel_jp(u8 first, u8 second);
    template<Flag if_set>
    u8 rel_jp_if(u8 first, u8 second);
    template<Flag not_set>
    u8 rel_jp_if_not(u8 first, u8 second);

    template<u8 address>
    u8 rst(u8 first, u8 second);
    u8 call(u8 first, u8 second);
    template<Flag if_set>
    u8 call_if(u8 first, u8 second);
    template<Flag not_set>
    u8 call_if_not(u8 first, u8 second);
    u8 ret(u8 first, u8 second);
    u8 reti(u8 first, u8 second);
    template<Flag if_set>
    u8 ret_if(u8 first, u8 second);
    template<Flag not_set>
    u8 ret_if_not(u8 first, u8 second);

    template<Reg16 reg>
    u8 pop_r(u8 first, u8 second);
    template<Reg16 reg>
    u8 push_r(u8 first, u8 second);
    
    template<ALU_Op operation, Reg8 other>
    constexpr u8 alu_a_r(u8 first, u8 second);
    template<ALU_Op operation>
    constexpr u8 alu_a_hli(u8 first, u8 second);
    template<ALU_Op operation>
    constexpr u8 alu_a_n(u8 first, u8 second);
    template<Reg8 reg>
    u8 inc_r(u8 first, u8 second);
    template<Reg8 reg>
    u8 dec_r(u8 first, u8 second);
    u8 inc_hli(u8 first, u8 second);
    u8 dec_hli(u8 first, u8 second);
    template<Reg16 reg>
    u8 inc_rr(u8 first, u8 second);
    template<Reg16 reg>
    u8 dec_rr(u8 first, u8 second);
    template<Reg16 reg>
    u8 add_hl_rr(u8 first, u8 second);
    u8 add_sp_n(u8 first, u8 second);
    u8 cpl(u8 first, u8 second);
    u8 ccf(u8 first, u8 second);
    u8 scf(u8 first, u8 second);
    u8 daa(u8 first, u8 second);

    template<bool state>
    u8 toggle_ints(u8 first, u8 second);

    u8 prefix_cb(u8 first, u8 second);

    template<u8 bit, Reg8 reg>
    void bit_n_r();
    template<u8 bit>
    void bit_n_hli();
    template<u8 bit, Reg8 reg>
    void res_n_r();
    template<u8 bit>
    void res_n_hli();
    template<u8 bit, Reg8 reg>
    void set_n_r();
    template<u8 bit>
    void set_n_hli();

    u8 rla(u8 first, u8 second);
    u8 rlca(u8 first, u8 second);
    u8 rra(u8 first, u8 second);
    u8 rrca(u8 first, u8 second);
    template<Reg8 reg>
    void rlc();
    void rlc_hli();
    template<Reg8 reg>
    void rl();
    void rl_hli();
    template<Reg8 reg>
    void rrc();
    void rrc_hli();
    template<Reg8 reg>
    void rr();
    void rr_hli();
    template<Reg8 reg>
    void sla();
    void sla_hli();
    template<Reg8 reg>
    void sra();
    void sra_hli();
    template<Reg8 reg>
    void srl();
    void srl_hli();
    template<Reg8 reg>
    void swap();
    void swap_hli();

    static std::array<u8 (CPU::*)(u8 first, u8 second), 256> m_opcodes;
    static std::array<void (CPU::*)(), 256> m_cb_opcodes;

    //Registers
    Reg af, bc, de, hl, sp, pc;

    //Memory and Cartridge
    Memory &m_mem;
    u8 m_opcode;

    //Other stuff
    Clock &m_clock;
    bool m_ime;
    bool m_halted;
    bool m_stopped;

    GB_MODEL &m_model;
    bool m_skip_bootrom;

    Reg& get_reg16(Reg16 reg);
    u8& get_reg8(Reg8 reg);
    bool get_flag(Flag flag);
    void set_flags(u8 flag);
    void reset_flags(u8 flag);
    void set_flags_if(u8 flag, bool value);

    void push(u16 address);
    u16 pop();


    //Logging
    std::ofstream m_log;

public:

    CPU(Memory &mem, Clock &clock, GB_MODEL &model, bool skip_bootrom = true);
    ~CPU();

    void service_interrupts();
    void request_interrupt(Interrupt type);
    void step();
    void reset();
    void nop() { m_clock.add_m(1); }
    void log_info();
    
    bool halted() { return m_halted; }
    bool stopped() { return m_stopped; }
    void un_stop() { m_stopped = false; }
    usize get_m_cycles() { return m_clock.get_m(); }

    friend class Memory;
};

} //namespace sb

#endif //CPU_HPP