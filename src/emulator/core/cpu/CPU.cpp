#include "CPU.hpp"

#include <iostream>

namespace sb {

CPU::CPU(Memory &mem, Clock &clock, GB_MODEL &model, bool skip_bootrom) : m_mem(mem), m_clock(clock), m_model(model), m_skip_bootrom(skip_bootrom) {
    reset();
}

CPU::~CPU() { }

Reg& CPU::get_reg16(Reg16 reg) {
    switch(reg) {
        case AF : return af;
        case BC : return bc;
        case DE : return de;
        case HL : return hl;
        case SP : return sp;
        case PC : return pc;
        default : LOG_FATAL("Unknown 16-bit register!");
    }
}

u8& CPU::get_reg8(Reg8 reg) {
    switch(reg) {
        case A : return af.hi;
        case F : return af.lo;
        case B : return bc.hi;
        case C : return bc.lo;
        case D : return de.hi;
        case E : return de.lo;
        case H : return hl.hi;
        case L : return hl.lo;
        default : LOG_FATAL("Unknown 16-bit register!");
    }
}

bool CPU::get_flag(Flag flag) {
    switch(flag) {
        case ZERO : return af.lo >> 7 & 1;
        case SUBTRACTION : return af.lo >> 6 & 1;
        case HALF_CARRY : return af.lo >> 5 & 1;
        case CARRY : return af.lo >> 4 & 1;
        default : return 0;
    }
}

void CPU::set_flags(u8 flag) {
    af.lo |= flag;
}

void CPU::reset_flags(u8 flag) {
    af.lo &= ~flag;
}

void CPU::set_flags_if(u8 flag, bool value) {
    if(value) {
        set_flags(flag);
    } else {
        reset_flags(flag);
    }
}

void CPU::push(u16 address) {
    sp.value -= 2;
    m_mem.write(sp.value, address & 0xff);
    m_mem.write(sp.value + 1, address >> 8);
}

u16 CPU::pop() {
    u16 address = (m_mem.read(sp.value + 1) << 8) | m_mem.read(sp.value);
    sp.value += 2;
    return address;
}

void CPU::request_interrupt(Interrupt type) {
    u8 int_f = m_mem.read(0xFF0F);
    m_mem.write(0xFF0F, int_f | type);
}

void CPU::service_interrupts() {
    u8 int_f = m_mem.read(0xFF0F);
    u8 int_e = m_mem.read(0xFFFF);
    u8 ints = int_f & int_e & 0x1f;
    if(ints != 0) {
        m_halted = false; //Apparently IME doesn't matter to stop halting
        m_stopped = false;

        if(m_ime) {
            bool vblank = ints & VBLANK_INT;
            bool lcd_stat = ints & LCD_STAT_INT;
            bool timer = ints & TIMER_INT;
            bool serial = ints & SERIAL_INT;
            bool joypad = ints & JOYPAD_INT;

            static const u8 int_vectors[5] = {0x40, 0x48, 0x50, 0x58, 0x60};
            bool enabled[5] = {vblank, lcd_stat, timer, serial, joypad};
            for(int i = 0; i < 5; i++) {
                if(enabled[i]) {
                    m_clock.add_m(5);
                    //Disable the interrupts
                    m_mem.write(0xFF0F, int_f & ~(1 << i));
                    m_ime = false;

                    //Push current pc onto the stack
                    push(pc.value);
                    
                    //Set pc to interrupt vector
                    pc.value = int_vectors[i];

                    break;
                }
            }
        }
    }
}

void CPU::step() {
    m_clock.add_m(1);
    m_opcode = m_mem.read(pc.value);
    u8 op1 = m_mem.read(pc.value + 1);
    u8 op2 = m_mem.read(pc.value + 2);
    u8 num_operands = (this->*m_opcodes[m_opcode])(op1, op2);

    pc.value += 1 + num_operands;

    //Logging
    // if(m_mem.read(0xFF50) == 1 && log) {
    //     m_log << fmt::format("A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} L: {:02X} SP: {:04X} PC: 00:{:04X} ({:02X} {:02X} {:02X} {:02X})\n",
    //     af.hi, af.lo, bc.hi, bc.lo, de.hi, de.lo, hl.hi, hl.lo, sp.value, pc.value, m_mem.read(pc.value), m_mem.read(pc.value + 1), m_mem.read(pc.value + 2), m_mem.read(pc.value + 3));
    // }
}

void CPU::reset() {
    m_ime = false;
    m_halted = false;
    m_stopped = false;

    if(m_skip_bootrom) {
        if(m_model == DMG) {
            af.value = 0x01B0;
            bc.value = 0x0013;
            de.value = 0x00D8;
            hl.value = 0x014D;
            sp.value = 0xFFFE;
            pc.value = 0x0100;
            m_mem.write(0xFF00 + 0x50, 1);
        } else {
            af.value = 0x1180;
            bc.value = 0x0000;
            de.value = 0xFF56;
            hl.value = 0x000D;
            sp.value = 0xFFFE;
            pc.value = 0x0100;
            m_mem.write(0xFF00 + 0x50, 1);
        }
    } else {
        af.value = 0;
        bc.value = 0;
        de.value = 0;
        hl.value = 0;
        sp.value = 0;
        pc.value = 0;
    }
}

} //namespace sb