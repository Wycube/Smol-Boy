#include "CPU.hpp"

namespace sb {

u8 CPU::illegal(u8 first, u8 second) {
    LOG_FATAL("Illegal opcode!");
    return 0;
}

u8 CPU::unknown(u8 first, u8 second) {
    std::string format = fmt::format("Unknown opcode 0x{1:02X} at pc = 0x{0:04X}! [{2}]", pc.value, m_opcode, mnemonic[m_opcode]);
    LOG_FATAL(format, first, to_u16(second, first), (s8)first);
    return 0;
}

void CPU::cb_unknown() {
    u8 cb_opcode = m_mem.read(pc.value + 1);
    std::string format = fmt::format("Unknown opcode 0x{1:02X} at pc = 0x{0:04X}! [{2}]", pc.value + 1, cb_opcode, cb_mnemonic[cb_opcode]);
    LOG_FATAL(format, 0, 0);
}

u8 CPU::nop(u8 first, u8 second) {
    return 0;
}

u8 CPU::halt(u8 first, u8 second) {
    m_halted = true;
    return 0;
}

u8 CPU::stop(u8 first, u8 second) {
    m_stopped = true;
    LOG_INFO("Stopped = true");
    return 0;
}

template<Reg8 dest, Reg8 src>
u8 CPU::ld_r_r(u8 first, u8 second) {
    //LOG_INFO("dest = {}, src = {}", get_reg8(dest), get_reg8(src));
    get_reg8(dest) = get_reg8(src);
    //LOG_INFO("dest = {}, src = {}", get_reg8(dest), get_reg8(src));
    return 0;
}

template<Reg8 dest>
u8 CPU::ld_r_hl(u8 first, u8 second) {
    m_clock.add_m(1);
    get_reg8(dest) = m_mem.read(hl.value);
    return 0;
}

template<Reg8 src>
u8 CPU::ld_hl_r(u8 first, u8 second) {
    m_clock.add_m(1);
    m_mem.write(hl.value, get_reg8(src));
    return 0;
}

template<Reg8 dest>
u8 CPU::ld_r_n(u8 first, u8 second) {
    m_clock.add_m(1);
    get_reg8(dest) = first;
    return 1;
}

u8 CPU::ld_hl_n(u8 first, u8 second) {
    m_clock.add_m(2);
    m_mem.write(hl.value, first);
    return 1;
}

template<Reg16 src_indirect>
u8 CPU::ld_a_ri(u8 first, u8 second) {
    m_clock.add_m(1);
    get_reg8(A) = m_mem.read(get_reg16(src_indirect).value);
    return 0;
}

template<Reg16 dest_indirect>
u8 CPU::ld_ri_a(u8 first, u8 second) {
    m_clock.add_m(1);
    m_mem.write(get_reg16(dest_indirect).value, get_reg8(A));
    return 0;
}

template<s8 adder>
u8 CPU::ld_hli_a(u8 first, u8 second) {
    m_clock.add_m(1);
    m_mem.write(hl.value, get_reg8(A));
    hl.value += adder;
    return 0;
}

template<s8 adder>
u8 CPU::ld_a_hli(u8 first, u8 second) {
    m_clock.add_m(1);
    get_reg8(A) = m_mem.read(hl.value);
    hl.value += adder;
    return 0;
}

template<Reg16 dest>
u8 CPU::ld_rr_nn(u8 first, u8 second) {
    m_clock.add_m(2);
    //LOG_INFO("Reg={:04X}, 0x{:04X} = {:02X}", get_reg16(dest).value, to_u16(second, first), m_mem.read(to_u16(second, first)));
    get_reg16(dest).value = to_u16(second, first); //Values stored in little-endian
    //LOG_INFO("Reg={:04X}, 0x{:04X} = {:02X}", get_reg16(dest).value, to_u16(second, first), m_mem.read(to_u16(second, first)));
    return 2;
}

u8 CPU::ld_nn_sp(u8 first, u8 second) {
    m_clock.add_m(4);
    u16 address = to_u16(second, first);
    m_mem.write(address, sp.lo);
    m_mem.write(address + 1, sp.hi);
    return 2;
}

u8 CPU::read_io_n(u8 first, u8 second) {
    m_clock.add_m(2);

    get_reg8(A) = m_mem.read(0xFF00 + first);

    return 1;
}

u8 CPU::write_io_n(u8 first, u8 second) {
    m_clock.add_m(2);

    m_mem.write(0xFF00 + first, get_reg8(A));

    return 1;
}

u8 CPU::read_io_c(u8 first, u8 second) {
    m_clock.add_m(1);

    get_reg8(A) = m_mem.read(0xFF00 + get_reg8(C));

    return 0;
}

u8 CPU::write_io_c(u8 first, u8 second) {
    m_clock.add_m(1);

    m_mem.write(0xFF00 + get_reg8(C), get_reg8(A));

    return 0;
}

u8 CPU::ld_nn_a(u8 first, u8 second) {
    m_clock.add_m(3);

    m_mem.write(to_u16(second, first), get_reg8(A));

    return 2;
}

u8 CPU::ld_a_nn(u8 first, u8 second) {
    m_clock.add_m(3);

    get_reg8(A) = m_mem.read(to_u16(second, first));

    return 2;
}

u8 CPU::ld_sp_hl(u8 first, u8 second) {
    m_clock.add_m(1);
    sp.value = hl.value;
    
    return 0;
}

u8 CPU::ld_hl_spn(u8 first, u8 second) {
    m_clock.add_m(2);
    hl.value = sp.value + (s8)first;

    reset_flags(ZERO | SUBTRACTION);
    set_flags_if(HALF_CARRY, (sp.value & 0xf) + (first & 0xf) > 0xf);
    set_flags_if(CARRY, (sp.value & 0xff) + (first & 0xff) > 0xff);

    return 1;
}

u8 CPU::abs_jp(u8 first, u8 second) {
    m_clock.add_m(3);
    u16 address = to_u16(second, first);
    pc.value = address - 1;
    return 0;
}

template<Flag flag>
u8 CPU::abs_jp_if(u8 first, u8 second) {
    m_clock.add_m(2);
    u16 address = to_u16(second, first);
    
    if(get_flag(flag)) {
        m_clock.add_m(1);
        pc.value = address - 1;
        return 0;
    }

    return 2;
}

template<Flag flag>
u8 CPU::abs_jp_if_not(u8 first, u8 second) {
    m_clock.add_m(2);
    u16 address = to_u16(second, first);
    
    if(!get_flag(flag)) {
        m_clock.add_m(1);
        pc.value = address - 1;
        return 0;
    }

    return 2;
}

u8 CPU::abs_jp_hl(u8 first, u8 second) {
    pc.value = hl.value - 1;

    return 0;
}

u8 CPU::rel_jp(u8 first, u8 second) {
    m_clock.add_m(2);
    pc.value += (s8)first + 1;
    return 0;
}

template<Flag flag>
u8 CPU::rel_jp_if(u8 first, u8 second) {
    m_clock.add_m(1);
    
    if(get_flag(flag)) {
        m_clock.add_m(1);
        pc.value += (s8)first + 1;
        return 0;
    }

    return 1;
}

template<Flag flag>
u8 CPU::rel_jp_if_not(u8 first, u8 second) {
    m_clock.add_m(1);
    
    if(!get_flag(flag)) {
        m_clock.add_m(1);
        pc.value += (s8)first + 1;
        return 0;
    }

    return 1;
}

template<u8 address>
u8 CPU::rst(u8 first, u8 second) {
    m_clock.add_m(3);
    push(pc.value + 1);
    pc.value = address - 1;
    return 0;
}

u8 CPU::call(u8 first, u8 second) {
    m_clock.add_m(5);
    push(pc.value + 3);
    pc.value = to_u16(second, first) - 1;
    return 0;
}

template<Flag if_set>
u8 CPU::call_if(u8 first, u8 second) {
    m_clock.add_m(2);

    if(get_flag(if_set)) {
        m_clock.add_m(3);
        push(pc.value + 3);
        pc.value = to_u16(second, first) - 1;
        return 0;
    }

    return 2;
}

template<Flag not_set>
u8 CPU::call_if_not(u8 first, u8 second) {
    m_clock.add_m(2);

    if(!get_flag(not_set)) {
        m_clock.add_m(3);
        push(pc.value + 3);
        pc.value = to_u16(second, first) - 1;
        return 0;
    }

    return 2;
}

u8 CPU::ret(u8 first, u8 second) {
    m_clock.add_m(3);
    pc.value = pop() - 1;
    
    return 0;
}

u8 CPU::reti(u8 first, u8 second) {
    m_clock.add_m(3);
    pc.value = pop() - 1;
    m_ime = true;

    return 0;
}

template<Flag if_set>
u8 CPU::ret_if(u8 first, u8 second) {
    m_clock.add_m(1);

    if(get_flag(if_set)) {
        m_clock.add_m(3);
        pc.value = pop() - 1;
    }
    
    return 0;
}

template<Flag not_set>
u8 CPU::ret_if_not(u8 first, u8 second) {
    m_clock.add_m(1);

    if(!get_flag(not_set)) {
        m_clock.add_m(3);
        pc.value = pop() - 1;
    }
    
    return 0;
}

template<Reg16 reg>
u8 CPU::pop_r(u8 first, u8 second) {
    m_clock.add_m(2);
    get_reg16(reg).value = pop();
    af.value &= 0xfff0; //Don't set flags that are impossible to set
    
    return 0;
}

template<Reg16 reg>
u8 CPU::push_r(u8 first, u8 second) {
    m_clock.add_m(3);
    push(get_reg16(reg).value);

    return 0;
}

template<ALU_Op operation, Reg8 other>
constexpr u8 CPU::alu_a_r(u8 first, u8 second) {
    u8 old = 0;
    u8 sum = 0;
    u8 n = get_reg8(other); //So op a, a instructions work

    switch(operation) {
        case ADD : 
            old = get_reg8(A);
            get_reg8(A) += n;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION);
            sum = (old & 0xf) + (n & 0xf);
            set_flags_if(HALF_CARRY, (sum & 0x10) == 0x10);
            set_flags_if(CARRY, get_reg8(A) < old);
            break;
        case ADC :
            old = get_reg8(A);
            get_reg8(A) += n + get_flag(CARRY);

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION);
            sum = (old & 0xf) + (n & 0xf) + get_flag(CARRY);
            set_flags_if(HALF_CARRY, sum > 0x0f);
            set_flags_if(CARRY, old + n + get_flag(CARRY) > 0xff);
            break;
        case SUB :
            old = get_reg8(A);
            get_reg8(A) -= n;

            set_flags_if(ZERO, get_reg8(A) == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (n & 0xf));
            set_flags_if(CARRY, old < n);
            break;
        case SBC :
            old = get_reg8(A);
            get_reg8(A) -= (n + get_flag(CARRY));

            set_flags_if(ZERO, get_reg8(A) == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (n & 0xf) + get_flag(CARRY));
            set_flags_if(CARRY, old < n + get_flag(CARRY));
            break;
        case AND :
            get_reg8(A) &= n;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | CARRY);
            set_flags(HALF_CARRY);
            break;
        case XOR :
            get_reg8(A) ^= n;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
            break;
        case  OR :
            get_reg8(A) |= n;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
            break;
        case  CP :
            old = get_reg8(A);
            sum = get_reg8(A) - n;

            set_flags_if(ZERO, sum == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (n & 0xf));
            set_flags_if(CARRY, old < n);
            break;
    }

    return 0;
}

template<ALU_Op operation>
constexpr u8 CPU::alu_a_hli(u8 first, u8 second) {
    u8 old = 0;
    u8 sum = 0;
    u8 hli = m_mem.read(hl.value);

    switch(operation) {
        case ADD : 
            old = get_reg8(A);
            get_reg8(A) += hli;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION);
            sum = (old & 0xf) + (hli & 0xf);
            set_flags_if(HALF_CARRY, (sum & 0x10) == 0x10);
            set_flags_if(CARRY, get_reg8(A) < old);
            break;
        case ADC :
            old = get_reg8(A);
            get_reg8(A) += hli + get_flag(CARRY);

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION);
            sum = (old & 0xf) + (hli & 0xf) + get_flag(CARRY);
            set_flags_if(HALF_CARRY, sum > 0x0f);
            set_flags_if(CARRY, old + hli + get_flag(CARRY) > 0xff);
            break;
        case SUB :
            old = get_reg8(A);
            get_reg8(A) -= hli;

            set_flags_if(ZERO, get_reg8(A) == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (hli & 0xf));
            set_flags_if(CARRY, old < hli);
            break;
        case SBC :
            old = get_reg8(A);
            get_reg8(A) -= (hli + get_flag(CARRY));

            set_flags_if(ZERO, get_reg8(A) == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (hli & 0xf) + get_flag(CARRY));
            set_flags_if(CARRY, old < hli + get_flag(CARRY));
            break;
        case AND :
            get_reg8(A) &= hli;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | CARRY);
            set_flags(HALF_CARRY);
            break;
        case XOR :
            get_reg8(A) ^= hli;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
            break;
        case  OR :
            get_reg8(A) |= hli;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
            break;
        case  CP :
            old = get_reg8(A);
            sum = get_reg8(A) - hli;

            set_flags_if(ZERO, sum == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (hli & 0xf));
            set_flags_if(CARRY, old < hli);
            break;
    }

    return 0;
}

template<ALU_Op operation>
constexpr u8 CPU::alu_a_n(u8 first, u8 second) {
    u8 old = 0;
    u8 sum = 0;

    switch(operation) {
        case ADD : 
            old = get_reg8(A);
            get_reg8(A) += first;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION);
            sum = (old & 0xf) + (first & 0xf);
            set_flags_if(HALF_CARRY, (sum & 0x10) == 0x10);
            set_flags_if(CARRY, get_reg8(A) < old);
            break;
        case ADC :
            old = get_reg8(A);
            get_reg8(A) += first + get_flag(CARRY);

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION);
            sum = (old & 0xf) + (first & 0xf) + get_flag(CARRY);
            set_flags_if(HALF_CARRY, sum > 0x0f);
            set_flags_if(CARRY, old + first + get_flag(CARRY) > 0xff);
            break;
        case SUB :
            old = get_reg8(A);
            get_reg8(A) -= first;

            set_flags_if(ZERO, get_reg8(A) == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (first & 0xf));
            set_flags_if(CARRY, old < first);
            break;
        case SBC :
            old = get_reg8(A);
            get_reg8(A) -= (first + get_flag(CARRY));

            set_flags_if(ZERO, get_reg8(A) == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (first & 0xf) + get_flag(CARRY));
            set_flags_if(CARRY, old < first + get_flag(CARRY));
            break;
        case AND :
            get_reg8(A) &= first;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | CARRY);
            set_flags(HALF_CARRY);
            break;
        case XOR :
            get_reg8(A) ^= first;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
            break;
        case  OR :
            get_reg8(A) |= first;

            set_flags_if(ZERO, get_reg8(A) == 0);
            reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
            break;
        case  CP :
            old = get_reg8(A);
            sum = get_reg8(A) - first;

            set_flags_if(ZERO, sum == 0);
            set_flags(SUBTRACTION);
            set_flags_if(HALF_CARRY, (old & 0xf) < (first & 0xf));
            set_flags_if(CARRY, old < first);
            break;
    }

    return 1;
}

template<Reg8 reg>
u8 CPU::inc_r(u8 first, u8 second) {
    get_reg8(reg)++;

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION);
    set_flags_if(HALF_CARRY, (get_reg8(reg) & 0xf) == 0);

    return 0;
}

template<Reg8 reg>
u8 CPU::dec_r(u8 first, u8 second) {
    set_flags_if(HALF_CARRY, (get_reg8(reg) & 0xf) < 1);

    get_reg8(reg)--;

    set_flags_if(ZERO, get_reg8(reg) == 0);
    set_flags(SUBTRACTION);

    return 0;
}

u8 CPU::inc_hli(u8 first, u8 second) {
    m_clock.add_m(2);
    u8 value = m_mem.read(hl.value);
    m_mem.write(hl.value, value + 1);

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION);
    set_flags_if(HALF_CARRY, (m_mem.read(hl.value) & 0xf) == 0);

    return 0;
}

u8 CPU::dec_hli(u8 first, u8 second) {
    m_clock.add_m(2);
    u8 value = m_mem.read(hl.value);
    set_flags_if(HALF_CARRY, (value & 0xf) < 1);

    m_mem.write(hl.value, value - 1);

    set_flags_if(ZERO, value - 1 == 0);
    set_flags(SUBTRACTION);

    return 0;
}

template<Reg16 reg>
u8 CPU::inc_rr(u8 first, u8 second) {
    m_clock.add_m(1);
    get_reg16(reg).value += 1;

    return 0;
}

template<Reg16 reg>
u8 CPU::dec_rr(u8 first, u8 second) {
    m_clock.add_m(1);
    get_reg16(reg).value -= 1;

    return 0;
}

template<Reg16 reg>
u8 CPU::add_hl_rr(u8 first, u8 second) {
    m_clock.add_m(1);
    Reg old_hl = hl;
    u16 rr = get_reg16(reg).value;
    hl.value += rr;

    reset_flags(SUBTRACTION);
    set_flags_if(HALF_CARRY, (old_hl.value & 0xfff) + (rr & 0xfff) & 0x1000);
    set_flags_if(CARRY, ((u32)old_hl.value + (u32)rr) & 0x10000);

    return 0;
}

//This dang instruction caused me to fail one of blargg's test, two to hang, and one to keep printing the name of the test over and over again.
u8 CPU::add_sp_n(u8 first, u8 second) {
    m_clock.add_m(3);
    u16 old_sp = sp.value;
    sp.value += (s8)first;

    reset_flags(ZERO | SUBTRACTION);
    set_flags_if(HALF_CARRY, (old_sp & 0xf) + ((s8)first & 0xf) > 0xf);
    set_flags_if(CARRY, (old_sp & 0xff) + ((s8)first & 0xff) > 0xff);

    return 1;
}

u8 CPU::cpl(u8 first, u8 second) {
    af.hi = ~af.hi;

    set_flags(SUBTRACTION | HALF_CARRY);

    return 0;
}

u8 CPU::ccf(u8 first, u8 second) {
    set_flags_if(CARRY, !get_flag(CARRY));
    reset_flags(SUBTRACTION | HALF_CARRY);

    return 0;
}

u8 CPU::scf(u8 first, u8 second) {
    set_flags(CARRY);
    reset_flags(SUBTRACTION | HALF_CARRY);

    return 0;
}

u8 CPU::daa(u8 first, u8 second) {
    //Adjust the A register so it's valid BCD
    if(!get_flag(SUBTRACTION)) {
        //Adjusts if half-carry or carry flag is set or if the result is out of bounds
        if(get_flag(CARRY) || get_reg8(A) > 0x99) { get_reg8(A) += 0x60; set_flags(CARRY); }
        if(get_flag(HALF_CARRY) || (get_reg8(A) & 0xf) > 0x09) { get_reg8(A) += 0x6; }
    } else {
        //Adjust if half-carry or carry flag is set
        if(get_flag(CARRY)) { get_reg8(A) -= 0x60; }
        if(get_flag(HALF_CARRY)) { get_reg8(A) -= 0x6; }
    }

    set_flags_if(ZERO, get_reg8(A) == 0);
    reset_flags(HALF_CARRY);

    return 0;
}

template<bool state>
u8 CPU::toggle_ints(u8 first, u8 second) {
    m_ime = state;

    return 0;
}

u8 CPU::prefix_cb(u8 first, u8 second) {
    m_clock.add_m(1);
    (this->*m_cb_opcodes[first])();
    return 1;
}

template<u8 bit, Reg8 reg>
void CPU::bit_n_r() {
    m_clock.add_m(1);
    set_flags_if(ZERO, ((get_reg8(reg) >> bit) & 1) == 0);
    set_flags(HALF_CARRY);
    reset_flags(SUBTRACTION);
}

template<u8 bit>
void CPU::bit_n_hli() {
    m_clock.add_m(2);
    set_flags_if(ZERO, ((m_mem.read(hl.value) >> bit) & 1) == 0);
    set_flags(HALF_CARRY);
    reset_flags(SUBTRACTION);
}

template<u8 bit, Reg8 reg>
void CPU::res_n_r() {
    m_clock.add_m(1);

    get_reg8(reg) &= ~(1 << bit);
}

template<u8 bit>
void CPU::res_n_hli() {
    m_clock.add_m(3);

    m_mem.write(hl.value, m_mem.read(hl.value) & ~(1 << bit));
}

template<u8 bit, Reg8 reg>
void CPU::set_n_r() {
    m_clock.add_m(1);

    get_reg8(reg) |= (1 << bit);
}

template<u8 bit>
void CPU::set_n_hli() {
    m_clock.add_m(3);

    m_mem.write(hl.value, m_mem.read(hl.value) | (1 << bit));
}

u8 CPU::rla(u8 first, u8 second) {
    u8 bit_7 = get_reg8(A) >> 7;
    get_reg8(A) = get_reg8(A) << 1 | get_flag(CARRY);

    set_flags_if(CARRY, bit_7);
    reset_flags(ZERO | SUBTRACTION | HALF_CARRY);

    return 0;
}

u8 CPU::rlca(u8 first, u8 second) {
    u8 bit_7 = get_reg8(A) >> 7;
    get_reg8(A) = get_reg8(A) << 1 | bit_7;

    set_flags_if(CARRY, bit_7);
    reset_flags(ZERO | SUBTRACTION | HALF_CARRY);

    return 0;
}

u8 CPU::rra(u8 first, u8 second) {
    u8 bit_0 = get_reg8(A) & 1;
    get_reg8(A) = get_reg8(A) >> 1 | (get_flag(CARRY) << 7);

    set_flags_if(CARRY, bit_0);
    reset_flags(ZERO | SUBTRACTION | HALF_CARRY);

    return 0;
}

u8 CPU::rrca(u8 first, u8 second) {
    u8 bit_0 = get_reg8(A) & 1;
    get_reg8(A) = get_reg8(A) >> 1 | (bit_0) << 7;

    set_flags_if(CARRY, bit_0);
    reset_flags(ZERO | SUBTRACTION | HALF_CARRY);

    return 0;
}

template<Reg8 reg>
void CPU::rlc() {
    m_clock.add_m(1);
    u8 bit_7 = get_reg8(reg) >> 7;
    get_reg8(reg) = get_reg8(reg) << 1 | bit_7;

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_7);
}

void CPU::rlc_hli() {
    m_clock.add_m(2);
    u8 bit_7 = m_mem.read(hl.value) >> 7;
    m_mem.write(hl.value, m_mem.read(hl.value) << 1 | bit_7);

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_7);
}

template<Reg8 reg>
void CPU::rl() {
    m_clock.add_m(1);
    u8 bit_7 = get_reg8(reg) >> 7;
    get_reg8(reg) = get_reg8(reg) << 1 | get_flag(CARRY);

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_7);
}

void CPU::rl_hli() {
    m_clock.add_m(2);
    u8 bit_7 = m_mem.read(hl.value) >> 7;
    m_mem.write(hl.value, m_mem.read(hl.value) << 1 | get_flag(CARRY));

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_7);
}

template<Reg8 reg>
void CPU::rrc() {
    m_clock.add_m(1);
    u8 bit_0 = get_reg8(reg) & 1;
    get_reg8(reg) = get_reg8(reg) >> 1 | (bit_0 << 7);

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

void CPU::rrc_hli() {
    m_clock.add_m(2);
    u8 bit_0 = m_mem.read(hl.value) & 1;
    m_mem.write(hl.value, m_mem.read(hl.value) >> 1 | (bit_0 << 7));

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

template<Reg8 reg>
void CPU::rr() {
    m_clock.add_m(1);
    u8 bit_0 = get_reg8(reg) & 1;
    get_reg8(reg) = get_reg8(reg) >> 1 | (get_flag(CARRY) << 7);

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

void CPU::rr_hli() {
    m_clock.add_m(2);
    u8 bit_0 = m_mem.read(hl.value) & 1;
    m_mem.write(hl.value, m_mem.read(hl.value) >> 1 | (get_flag(CARRY) << 7));

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

template<Reg8 reg>
void CPU::sla() {
    m_clock.add_m(1);
    u8 bit_7 = get_reg8(reg) >> 7;
    get_reg8(reg) = get_reg8(reg) << 1 & 0xfe;

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_7);
}

void CPU::sla_hli() {
    m_clock.add_m(2);
    u8 bit_7 = m_mem.read(hl.value) >> 7;
    m_mem.write(hl.value, m_mem.read(hl.value) << 1 & 0xfe);

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_7);
}

template<Reg8 reg>
void CPU::sra() {
    m_clock.add_m(1);
    u8 bit_7 = get_reg8(reg) >> 7;
    u8 bit_0 = get_reg8(reg) & 1;
    get_reg8(reg) = get_reg8(reg) >> 1 | (bit_7 << 7);

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

void CPU::sra_hli() {
    m_clock.add_m(2);
    u8 bit_7 = m_mem.read(hl.value) >> 7;
    u8 bit_0 = m_mem.read(hl.value) & 1;
    m_mem.write(hl.value, m_mem.read(hl.value) >> 1 | (bit_7 << 7));

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

template<Reg8 reg>
void CPU::srl() {
    m_clock.add_m(1);
    u8 bit_0 = get_reg8(reg) & 1;
    get_reg8(reg) = get_reg8(reg) >> 1;

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

void CPU::srl_hli() {
    m_clock.add_m(2);
    u8 bit_0 = m_mem.read(hl.value) & 1;
    m_mem.write(hl.value, m_mem.read(hl.value) >> 1);

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY);
    set_flags_if(CARRY, bit_0);
}

template<Reg8 reg>
void CPU::swap() {
    m_clock.add_m(1);
    get_reg8(reg) = (get_reg8(reg) << 4) | (get_reg8(reg) >> 4);

    set_flags_if(ZERO, get_reg8(reg) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
}

void CPU::swap_hli() {
    m_clock.add_m(2);
    m_mem.write(hl.value, (m_mem.read(hl.value) << 4) | (m_mem.read(hl.value) >> 4));

    set_flags_if(ZERO, m_mem.read(hl.value) == 0);
    reset_flags(SUBTRACTION | HALF_CARRY | CARRY);
}


std::array<u8 (CPU::*)(u8 first, u8 second), 256> CPU::m_opcodes = {
    &CPU::nop,     //0x00
    &CPU::ld_rr_nn<BC>, //0x01
    &CPU::ld_ri_a<BC>, //0x02
    &CPU::inc_rr<BC>, //0x03
    &CPU::inc_r<B>, //0x04
    &CPU::dec_r<B>, //0x05
    &CPU::ld_r_n<B>, //0x06
    &CPU::rlca, //0x07
    &CPU::ld_nn_sp, //0x08
    &CPU::add_hl_rr<BC>, //0x09
    &CPU::ld_a_ri<BC>, //0x0A
    &CPU::dec_rr<BC>, //0x0B
    &CPU::inc_r<C>, //0x0C
    &CPU::dec_r<C>, //0x0D
    &CPU::ld_r_n<C>, //0x0E
    &CPU::rrca, //0x0F
    &CPU::stop, //0x10
    &CPU::ld_rr_nn<DE>, //0x11
    &CPU::ld_ri_a<DE>, //0x12
    &CPU::inc_rr<DE>, //0x13
    &CPU::inc_r<D>, //0x14
    &CPU::dec_r<D>, //0x15
    &CPU::ld_r_n<D>, //0x16
    &CPU::rla, //0x17
    &CPU::rel_jp, //0x18
    &CPU::add_hl_rr<DE>, //0x19
    &CPU::ld_a_ri<DE>, //0x1A
    &CPU::dec_rr<DE>, //0x1B
    &CPU::inc_r<E>, //0x1C
    &CPU::dec_r<E>, //0x1D
    &CPU::ld_r_n<E>, //0x1E
    &CPU::rra, //0x1F
    &CPU::rel_jp_if_not<ZERO>, //0x20
    &CPU::ld_rr_nn<HL>, //0x21
    &CPU::ld_hli_a<1>, //0x22
    &CPU::inc_rr<HL>, //0x23
    &CPU::inc_r<H>, //0x24
    &CPU::dec_r<H>, //0x25
    &CPU::ld_r_n<H>, //0x26
    &CPU::daa, //0x27
    &CPU::rel_jp_if<ZERO>, //0x28
    &CPU::add_hl_rr<HL>, //0x29
    &CPU::ld_a_hli<1>, //0x2A
    &CPU::dec_rr<HL>, //0x2B
    &CPU::inc_r<L>, //0x2C
    &CPU::dec_r<L>, //0x2D
    &CPU::ld_r_n<L>, //0x2E
    &CPU::cpl, //0x2F
    &CPU::rel_jp_if_not<CARRY>, //0x30
    &CPU::ld_rr_nn<SP>, //0x31
    &CPU::ld_hli_a<-1>, //0x32
    &CPU::inc_rr<SP>, //0x33
    &CPU::inc_hli, //0x34
    &CPU::dec_hli, //0x35
    &CPU::ld_hl_n, //0x36
    &CPU::scf, //0x37
    &CPU::rel_jp_if<CARRY>, //0x38
    &CPU::add_hl_rr<SP>, //0x39
    &CPU::ld_a_hli<-1>, //0x3A
    &CPU::dec_rr<SP>, //0x3B
    &CPU::inc_r<A>, //0x3C
    &CPU::dec_r<A>, //0x3D
    &CPU::ld_r_n<A>, //0x3E
    &CPU::ccf, //0x3F
    &CPU::ld_r_r<B, B>, //0x40
    &CPU::ld_r_r<B, C>, //0x41
    &CPU::ld_r_r<B, D>, //0x42
    &CPU::ld_r_r<B, E>, //0x43
    &CPU::ld_r_r<B, H>, //0x44
    &CPU::ld_r_r<B, L>, //0x45
    &CPU::ld_r_hl<B>, //0x46
    &CPU::ld_r_r<B, A>, //0x47
    &CPU::ld_r_r<C, B>, //0x48
    &CPU::ld_r_r<C, C>, //0x49
    &CPU::ld_r_r<C, D>, //0x4A
    &CPU::ld_r_r<C, E>, //0x4B
    &CPU::ld_r_r<C, H>, //0x4C
    &CPU::ld_r_r<C, L>, //0x4D
    &CPU::ld_r_hl<C>, //0x4E
    &CPU::ld_r_r<C, A>, //0x4F
    &CPU::ld_r_r<D, B>, //0x50
    &CPU::ld_r_r<D, C>, //0x51
    &CPU::ld_r_r<D, D>, //0x52
    &CPU::ld_r_r<D, E>, //0x53
    &CPU::ld_r_r<D, H>, //0x54
    &CPU::ld_r_r<D, L>, //0x55
    &CPU::ld_r_hl<D>, //0x56
    &CPU::ld_r_r<D, A>, //0x57
    &CPU::ld_r_r<E, B>, //0x58
    &CPU::ld_r_r<E, C>, //0x59
    &CPU::ld_r_r<E, D>, //0x5A
    &CPU::ld_r_r<E, E>, //0x5B
    &CPU::ld_r_r<E, H>, //0x5C
    &CPU::ld_r_r<E, L>, //0x5D
    &CPU::ld_r_hl<E>, //0x5E
    &CPU::ld_r_r<E, A>, //0x5F
    &CPU::ld_r_r<H, B>, //0x60
    &CPU::ld_r_r<H, C>, //0x61
    &CPU::ld_r_r<H, D>, //0x62
    &CPU::ld_r_r<H, E>, //0x63
    &CPU::ld_r_r<H, H>, //0x64
    &CPU::ld_r_r<H, L>, //0x65
    &CPU::ld_r_hl<H>, //0x66
    &CPU::ld_r_r<H, A>, //0x67
    &CPU::ld_r_r<L, B>, //0x68
    &CPU::ld_r_r<L, C>, //0x69
    &CPU::ld_r_r<L, D>, //0x6A
    &CPU::ld_r_r<L, E>, //0x6B
    &CPU::ld_r_r<L, H>, //0x6C
    &CPU::ld_r_r<L, L>, //0x6D
    &CPU::ld_r_hl<L>, //0x6E
    &CPU::ld_r_r<L, A>, //0x6F
    &CPU::ld_hl_r<B>, //0x70
    &CPU::ld_hl_r<C>, //0x71
    &CPU::ld_hl_r<D>, //0x72
    &CPU::ld_hl_r<E>, //0x73
    &CPU::ld_hl_r<H>, //0x74
    &CPU::ld_hl_r<L>, //0x75
    &CPU::halt, //0x76
    &CPU::ld_hl_r<A>, //0x77
    &CPU::ld_r_r<A, B>, //0x78
    &CPU::ld_r_r<A, C>, //0x79
    &CPU::ld_r_r<A, D>, //0x7A
    &CPU::ld_r_r<A, E>, //0x7B
    &CPU::ld_r_r<A, H>, //0x7C
    &CPU::ld_r_r<A, L>, //0x7D
    &CPU::ld_r_hl<A>, //0x7E
    &CPU::ld_r_r<A, A>, //0x7F
    &CPU::alu_a_r<ADD, B>, //0x80
    &CPU::alu_a_r<ADD, C>, //0x81
    &CPU::alu_a_r<ADD, D>, //0x82
    &CPU::alu_a_r<ADD, E>, //0x83
    &CPU::alu_a_r<ADD, H>, //0x84
    &CPU::alu_a_r<ADD, L>, //0x85
    &CPU::alu_a_hli<ADD>, //0x86
    &CPU::alu_a_r<ADD, A>, //0x87
    &CPU::alu_a_r<ADC, B>, //0x88
    &CPU::alu_a_r<ADC, C>, //0x89
    &CPU::alu_a_r<ADC, D>, //0x8A
    &CPU::alu_a_r<ADC, E>, //0x8B
    &CPU::alu_a_r<ADC, H>, //0x8C
    &CPU::alu_a_r<ADC, L>, //0x8D
    &CPU::alu_a_hli<ADC>, //0x8E
    &CPU::alu_a_r<ADC, A>, //0x8F
    &CPU::alu_a_r<SUB, B>, //0x90
    &CPU::alu_a_r<SUB, C>, //0x91
    &CPU::alu_a_r<SUB, D>, //0x92
    &CPU::alu_a_r<SUB, E>, //0x93
    &CPU::alu_a_r<SUB, H>, //0x94
    &CPU::alu_a_r<SUB, L>, //0x95
    &CPU::alu_a_hli<SUB>, //0x96
    &CPU::alu_a_r<SUB, A>, //0x97
    &CPU::alu_a_r<SBC, B>, //0x98
    &CPU::alu_a_r<SBC, C>, //0x99
    &CPU::alu_a_r<SBC, D>, //0x9A
    &CPU::alu_a_r<SBC, E>, //0x9B
    &CPU::alu_a_r<SBC, H>, //0x9C
    &CPU::alu_a_r<SBC, L>, //0x9D
    &CPU::alu_a_hli<SBC>, //0x9E
    &CPU::alu_a_r<SBC, A>, //0x9F
    &CPU::alu_a_r<AND, B>, //0xA0
    &CPU::alu_a_r<AND, C>, //0xA1
    &CPU::alu_a_r<AND, D>, //0xA2
    &CPU::alu_a_r<AND, E>, //0xA3
    &CPU::alu_a_r<AND, H>, //0xA4
    &CPU::alu_a_r<AND, L>, //0xA5
    &CPU::alu_a_hli<AND>, //0xA6
    &CPU::alu_a_r<AND, A>, //0xA7
    &CPU::alu_a_r<XOR, B>, //0xA8
    &CPU::alu_a_r<XOR, C>, //0xA9
    &CPU::alu_a_r<XOR, D>, //0xAA
    &CPU::alu_a_r<XOR, E>, //0xAB
    &CPU::alu_a_r<XOR, H>, //0xAC
    &CPU::alu_a_r<XOR, L>, //0xAD
    &CPU::alu_a_hli<XOR>, //0xAE
    &CPU::alu_a_r<XOR, A>, //0xAF
    &CPU::alu_a_r<OR, B>, //0xB0
    &CPU::alu_a_r<OR, C>, //0xB1
    &CPU::alu_a_r<OR, D>, //0xB2
    &CPU::alu_a_r<OR, E>, //0xB3
    &CPU::alu_a_r<OR, H>, //0xB4
    &CPU::alu_a_r<OR, L>, //0xB5
    &CPU::alu_a_hli<OR>, //0xB6
    &CPU::alu_a_r<OR, A>, //0xB7
    &CPU::alu_a_r<CP, B>, //0xB8
    &CPU::alu_a_r<CP, C>, //0xB9
    &CPU::alu_a_r<CP, D>, //0xBA
    &CPU::alu_a_r<CP, E>, //0xBB
    &CPU::alu_a_r<CP, H>, //0xBC
    &CPU::alu_a_r<CP, L>, //0xBD
    &CPU::alu_a_hli<CP>, //0xBE
    &CPU::alu_a_r<CP, A>, //0xBF
    &CPU::ret_if_not<ZERO>, //0xC0
    &CPU::pop_r<BC>, //0xC1
    &CPU::abs_jp_if_not<ZERO>, //0xC2
    &CPU::abs_jp, //0xC3
    &CPU::call_if_not<ZERO>, //0xC4
    &CPU::push_r<BC>, //0xC5
    &CPU::alu_a_n<ADD>, //0xC6
    &CPU::rst<0x00>, //0xC7
    &CPU::ret_if<ZERO>, //0xC8
    &CPU::ret, //0xC9
    &CPU::abs_jp_if<ZERO>, //0xCA
    &CPU::prefix_cb, //0xCB
    &CPU::call_if<ZERO>, //0xCC
    &CPU::call, //0xCD
    &CPU::alu_a_n<ADC>, //0xCE
    &CPU::rst<0x08>, //0xCF
    &CPU::ret_if_not<CARRY>, //0xD0
    &CPU::pop_r<DE>, //0xD1
    &CPU::abs_jp_if_not<CARRY>, //0xD2
    &CPU::illegal, //0xD3
    &CPU::call_if_not<CARRY>, //0xD4
    &CPU::push_r<DE>, //0xD5
    &CPU::alu_a_n<SUB>, //0xD6
    &CPU::rst<0x10>, //0xD7
    &CPU::ret_if<CARRY>, //0xD8
    &CPU::reti, //0xD9
    &CPU::abs_jp_if<CARRY>, //0xDA
    &CPU::illegal, //0xDB
    &CPU::call_if<CARRY>, //0xDC
    &CPU::illegal, //0xDD
    &CPU::alu_a_n<SBC>, //0xDE
    &CPU::rst<0x18>, //0xDF
    &CPU::write_io_n, //0xE0
    &CPU::pop_r<HL>, //0xE1
    &CPU::write_io_c, //0xE2
    &CPU::illegal, //0xE3
    &CPU::illegal, //0xE4
    &CPU::push_r<HL>, //0xE5
    &CPU::alu_a_n<AND>, //0xE6
    &CPU::rst<0x20>, //0xE7
    &CPU::add_sp_n, //0xE8
    &CPU::abs_jp_hl, //0xE9
    &CPU::ld_nn_a, //0xEA
    &CPU::illegal, //0xEB
    &CPU::illegal, //0xEC
    &CPU::illegal, //0xED
    &CPU::alu_a_n<XOR>, //0xEE
    &CPU::rst<0x28>, //0xEF
    &CPU::read_io_n, //0xF0
    &CPU::pop_r<AF>, //0xF1
    &CPU::read_io_c, //0xF2
    &CPU::toggle_ints<false>, //0xF3
    &CPU::illegal, //0xF4
    &CPU::push_r<AF>, //0xF5
    &CPU::alu_a_n<OR>, //0xF6
    &CPU::rst<0x30>, //0xF7
    &CPU::ld_hl_spn, //0xF8
    &CPU::ld_sp_hl, //0xF9
    &CPU::ld_a_nn, //0xFA
    &CPU::toggle_ints<true>, //0xFB
    &CPU::illegal, //0xFC
    &CPU::illegal, //0xFD
    &CPU::alu_a_n<CP>, //0xFE
    &CPU::rst<0x38>, //0xFF
};

std::array<void (CPU::*)(), 256> CPU::m_cb_opcodes = {
    &CPU::rlc<B>, //0x00
    &CPU::rlc<C>, //0x01
    &CPU::rlc<D>, //0x02
    &CPU::rlc<E>, //0x03
    &CPU::rlc<H>, //0x04
    &CPU::rlc<L>, //0x05
    &CPU::rlc_hli, //0x06
    &CPU::rlc<A>, //0x07
    &CPU::rrc<B>, //0x08
    &CPU::rrc<C>, //0x09
    &CPU::rrc<D>, //0x0A
    &CPU::rrc<E>, //0x0B
    &CPU::rrc<H>, //0x0C
    &CPU::rrc<L>, //0x0D
    &CPU::rrc_hli, //0x0E
    &CPU::rrc<A>, //0x0F
    &CPU::rl<B>, //0x10
    &CPU::rl<C>, //0x11
    &CPU::rl<D>, //0x12
    &CPU::rl<E>, //0x13
    &CPU::rl<H>, //0x14
    &CPU::rl<L>, //0x15
    &CPU::rl_hli, //0x16
    &CPU::rl<A>, //0x17
    &CPU::rr<B>, //0x18
    &CPU::rr<C>, //0x19
    &CPU::rr<D>, //0x1A
    &CPU::rr<E>, //0x1B
    &CPU::rr<H>, //0x1C
    &CPU::rr<L>, //0x1D
    &CPU::rr_hli, //0x1E
    &CPU::rr<A>, //0x1F
    &CPU::sla<B>, //0x20
    &CPU::sla<C>, //0x21
    &CPU::sla<D>, //0x22
    &CPU::sla<E>, //0x23
    &CPU::sla<H>, //0x24
    &CPU::sla<L>, //0x25
    &CPU::sla_hli, //0x26
    &CPU::sla<A>, //0x27
    &CPU::sra<B>, //0x28
    &CPU::sra<C>, //0x29
    &CPU::sra<D>, //0x2A
    &CPU::sra<E>, //0x2B
    &CPU::sra<H>, //0x2C
    &CPU::sra<L>, //0x2D
    &CPU::sra_hli, //0x2E
    &CPU::sra<A>, //0x2F
    &CPU::swap<B>, //0x30
    &CPU::swap<C>, //0x31
    &CPU::swap<D>, //0x32
    &CPU::swap<E>, //0x33
    &CPU::swap<H>, //0x34
    &CPU::swap<L>, //0x35
    &CPU::swap_hli, //0x36
    &CPU::swap<A>, //0x37
    &CPU::srl<B>, //0x38
    &CPU::srl<C>, //0x39
    &CPU::srl<D>, //0x3A
    &CPU::srl<E>, //0x3B
    &CPU::srl<H>, //0x3C
    &CPU::srl<L>, //0x3D
    &CPU::srl_hli, //0x3E
    &CPU::srl<A>, //0x3F
    &CPU::bit_n_r<0, B>, //0x40
    &CPU::bit_n_r<0, C>, //0x41
    &CPU::bit_n_r<0, D>, //0x42
    &CPU::bit_n_r<0, E>, //0x43
    &CPU::bit_n_r<0, H>, //0x44
    &CPU::bit_n_r<0, L>, //0x45
    &CPU::bit_n_hli<0>, //0x46
    &CPU::bit_n_r<0, A>, //0x47
    &CPU::bit_n_r<1, B>, //0x48
    &CPU::bit_n_r<1, C>, //0x49
    &CPU::bit_n_r<1, D>, //0x4A
    &CPU::bit_n_r<1, E>, //0x4B
    &CPU::bit_n_r<1, H>, //0x4C
    &CPU::bit_n_r<1, L>, //0x4D
    &CPU::bit_n_hli<1>, //0x4E
    &CPU::bit_n_r<1, A>, //0x4F
    &CPU::bit_n_r<2, B>, //0x50
    &CPU::bit_n_r<2, C>, //0x51
    &CPU::bit_n_r<2, D>, //0x52
    &CPU::bit_n_r<2, E>, //0x53
    &CPU::bit_n_r<2, H>, //0x54
    &CPU::bit_n_r<2, L>, //0x55
    &CPU::bit_n_hli<2>, //0x56
    &CPU::bit_n_r<2, A>, //0x57
    &CPU::bit_n_r<3, B>, //0x58
    &CPU::bit_n_r<3, C>, //0x59
    &CPU::bit_n_r<3, D>, //0x5A
    &CPU::bit_n_r<3, E>, //0x5B
    &CPU::bit_n_r<3, H>, //0x5C
    &CPU::bit_n_r<3, L>, //0x5D
    &CPU::bit_n_hli<3>, //0x5E
    &CPU::bit_n_r<3, A>, //0x5F
    &CPU::bit_n_r<4, B>, //0x60
    &CPU::bit_n_r<4, C>, //0x61
    &CPU::bit_n_r<4, D>, //0x62
    &CPU::bit_n_r<4, E>, //0x63
    &CPU::bit_n_r<4, H>, //0x64
    &CPU::bit_n_r<4, L>, //0x65
    &CPU::bit_n_hli<4>, //0x66
    &CPU::bit_n_r<4, A>, //0x67
    &CPU::bit_n_r<5, B>, //0x68
    &CPU::bit_n_r<5, C>, //0x69
    &CPU::bit_n_r<5, D>, //0x6A
    &CPU::bit_n_r<5, E>, //0x6B
    &CPU::bit_n_r<5, H>, //0x6C
    &CPU::bit_n_r<5, L>, //0x6D
    &CPU::bit_n_hli<5>, //0x6E
    &CPU::bit_n_r<5, A>, //0x6F
    &CPU::bit_n_r<6, B>, //0x70
    &CPU::bit_n_r<6, C>, //0x71
    &CPU::bit_n_r<6, D>, //0x72
    &CPU::bit_n_r<6, E>, //0x73
    &CPU::bit_n_r<6, H>, //0x74
    &CPU::bit_n_r<6, L>, //0x75
    &CPU::bit_n_hli<6>, //0x76
    &CPU::bit_n_r<6, A>, //0x77
    &CPU::bit_n_r<7, B>, //0x78
    &CPU::bit_n_r<7, C>, //0x79
    &CPU::bit_n_r<7, D>, //0x7A
    &CPU::bit_n_r<7, E>, //0x7B
    &CPU::bit_n_r<7, H>, //0x7C
    &CPU::bit_n_r<7, L>, //0x7D
    &CPU::bit_n_hli<7>, //0x7E
    &CPU::bit_n_r<7, A>, //0x7F
    &CPU::res_n_r<0, B>, //0x80
    &CPU::res_n_r<0, C>, //0x81
    &CPU::res_n_r<0, D>, //0x82
    &CPU::res_n_r<0, E>, //0x83
    &CPU::res_n_r<0, H>, //0x84
    &CPU::res_n_r<0, L>, //0x85
    &CPU::res_n_hli<0>, //0x86
    &CPU::res_n_r<0, A>, //0x87
    &CPU::res_n_r<1, B>, //0x88
    &CPU::res_n_r<1, C>, //0x89
    &CPU::res_n_r<1, D>, //0x8A
    &CPU::res_n_r<1, E>, //0x8B
    &CPU::res_n_r<1, H>, //0x8C
    &CPU::res_n_r<1, L>, //0x8D
    &CPU::res_n_hli<1>, //0x8E
    &CPU::res_n_r<1, A>, //0x8F
    &CPU::res_n_r<2, B>, //0x90
    &CPU::res_n_r<2, C>, //0x91
    &CPU::res_n_r<2, D>, //0x92
    &CPU::res_n_r<2, E>, //0x93
    &CPU::res_n_r<2, H>, //0x94
    &CPU::res_n_r<2, L>, //0x95
    &CPU::res_n_hli<2>, //0x96
    &CPU::res_n_r<2, A>, //0x97
    &CPU::res_n_r<3, B>, //0x98
    &CPU::res_n_r<3, C>, //0x99
    &CPU::res_n_r<3, D>, //0x9A
    &CPU::res_n_r<3, E>, //0x9B
    &CPU::res_n_r<3, H>, //0x9C
    &CPU::res_n_r<3, L>, //0x9D
    &CPU::res_n_hli<3>, //0x9E
    &CPU::res_n_r<3, A>, //0x9F
    &CPU::res_n_r<4, B>, //0xA0
    &CPU::res_n_r<4, C>, //0xA1
    &CPU::res_n_r<4, D>, //0xA2
    &CPU::res_n_r<4, E>, //0xA3
    &CPU::res_n_r<4, H>, //0xA4
    &CPU::res_n_r<4, L>, //0xA5
    &CPU::res_n_hli<4>, //0xA6
    &CPU::res_n_r<4, A>, //0xA7
    &CPU::res_n_r<5, B>, //0xA8
    &CPU::res_n_r<5, C>, //0xA9
    &CPU::res_n_r<5, D>, //0xAA
    &CPU::res_n_r<5, E>, //0xAB
    &CPU::res_n_r<5, H>, //0xAC
    &CPU::res_n_r<5, L>, //0xAD
    &CPU::res_n_hli<5>, //0xAE
    &CPU::res_n_r<5, A>, //0xAF
    &CPU::res_n_r<6, B>, //0xB0
    &CPU::res_n_r<6, C>, //0xB1
    &CPU::res_n_r<6, D>, //0xB2
    &CPU::res_n_r<6, E>, //0xB3
    &CPU::res_n_r<6, H>, //0xB4
    &CPU::res_n_r<6, L>, //0xB5
    &CPU::res_n_hli<6>, //0xB6
    &CPU::res_n_r<6, A>, //0xB7
    &CPU::res_n_r<7, B>, //0xB8
    &CPU::res_n_r<7, C>, //0xB9
    &CPU::res_n_r<7, D>, //0xBA
    &CPU::res_n_r<7, E>, //0xBB
    &CPU::res_n_r<7, H>, //0xBC
    &CPU::res_n_r<7, L>, //0xBD
    &CPU::res_n_hli<7>, //0xBE
    &CPU::res_n_r<7, A>, //0xBF
    &CPU::set_n_r<0, B>, //0xB0
    &CPU::set_n_r<0, C>, //0xC1
    &CPU::set_n_r<0, D>, //0xC2
    &CPU::set_n_r<0, E>, //0xC3
    &CPU::set_n_r<0, H>, //0xC4
    &CPU::set_n_r<0, L>, //0xC5
    &CPU::set_n_hli<0>, //0xC6
    &CPU::set_n_r<0, A>, //0xC7
    &CPU::set_n_r<1, B>, //0xC8
    &CPU::set_n_r<1, C>, //0xC9
    &CPU::set_n_r<1, D>, //0xCA
    &CPU::set_n_r<1, E>, //0xCB
    &CPU::set_n_r<1, H>, //0xCC
    &CPU::set_n_r<1, L>, //0xCD
    &CPU::set_n_hli<1>, //0xCE
    &CPU::set_n_r<1, A>, //0xCF
    &CPU::set_n_r<2, B>, //0xC0
    &CPU::set_n_r<2, C>, //0xD1
    &CPU::set_n_r<2, D>, //0xD2
    &CPU::set_n_r<2, E>, //0xD3
    &CPU::set_n_r<2, H>, //0xD4
    &CPU::set_n_r<2, L>, //0xD5
    &CPU::set_n_hli<2>, //0xD6
    &CPU::set_n_r<2, A>, //0xD7
    &CPU::set_n_r<3, B>, //0xD8
    &CPU::set_n_r<3, C>, //0xD9
    &CPU::set_n_r<3, D>, //0xDA
    &CPU::set_n_r<3, E>, //0xDB
    &CPU::set_n_r<3, H>, //0xDC
    &CPU::set_n_r<3, L>, //0xDD
    &CPU::set_n_hli<3>, //0xDE
    &CPU::set_n_r<3, A>, //0xDF
    &CPU::set_n_r<4, B>, //0xE0
    &CPU::set_n_r<4, C>, //0xE1
    &CPU::set_n_r<4, D>, //0xE2
    &CPU::set_n_r<4, E>, //0xE3
    &CPU::set_n_r<4, H>, //0xE4
    &CPU::set_n_r<4, L>, //0xE5
    &CPU::set_n_hli<4>, //0xE6
    &CPU::set_n_r<4, A>, //0xE7
    &CPU::set_n_r<5, B>, //0xE8
    &CPU::set_n_r<5, C>, //0xE9
    &CPU::set_n_r<5, D>, //0xEA
    &CPU::set_n_r<5, E>, //0xEB
    &CPU::set_n_r<5, H>, //0xEC
    &CPU::set_n_r<5, L>, //0xED
    &CPU::set_n_hli<5>, //0xEE
    &CPU::set_n_r<5, A>, //0xEF
    &CPU::set_n_r<6, B>, //0xF0
    &CPU::set_n_r<6, C>, //0xF1
    &CPU::set_n_r<6, D>, //0xF2
    &CPU::set_n_r<6, E>, //0xF3
    &CPU::set_n_r<6, H>, //0xF4
    &CPU::set_n_r<6, L>, //0xF5
    &CPU::set_n_hli<6>, //0xF6
    &CPU::set_n_r<6, A>, //0xF7
    &CPU::set_n_r<7, B>, //0xF8
    &CPU::set_n_r<7, C>, //0xF9
    &CPU::set_n_r<7, D>, //0xFA
    &CPU::set_n_r<7, E>, //0xFB
    &CPU::set_n_r<7, H>, //0xFC
    &CPU::set_n_r<7, L>, //0xFD
    &CPU::set_n_hli<7>, //0xFE
    &CPU::set_n_r<7, A>, //0xFF
};

} //namespace sb