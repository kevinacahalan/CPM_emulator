#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "portable.h"


struct cpu{
    unsigned short pc; // Instruction Pointer  /  Program Counter
    unsigned short sp; // stack pointer

    unsigned short ix; // index x
    unsigned short iy; // index y


    // main registers
    union{
        unsigned short af;
        struct{
            unsigned char f;
            unsigned char a;
        };
        //flags
        struct{
            unsigned short f_c : 1; // carry flag
            unsigned short f_n : 1; // 1 for addition, 0 for subtraction
            unsigned short f_pv : 1; // parity / overflow
            unsigned short f_bit3 : 1;
            unsigned short f_h : 1; // half carry
            unsigned short f_bit5 : 1;
            unsigned short f_z : 1; // zero flag
            unsigned short f_s : 1; // negative flag
            unsigned short f_ : 8;
        };
    };
    union{
        unsigned short bc;
        struct{
            unsigned char c;
            unsigned char b;
        };
    };
    union{
        unsigned short de;
        struct{
            unsigned char e;
            unsigned char d;
        };
    };
    union{
        unsigned short hl;
        struct{
            unsigned char l;
            unsigned char h;
        };
    };

    // Alternate registers
    unsigned short af_prime;
    unsigned short bc_prime;
    unsigned short de_prime;
    unsigned short hl_prime;
};

static unsigned char parity(unsigned char p){
    p = ((p >> 1) & 0x55)+(p & 0x55);
    p = ((p >> 2) & 0x33)+(p & 0x33);
    p = ((p >> 4) & 0x0f)+(p & 0x0f);
    p = !(p & 1);

    return p;
}

static unsigned alu_8_add(struct cpu *cpu, unsigned x, unsigned y, unsigned carry_in){

    uint64_t hsum = (x & 0xf) + (y & 0xf) + (carry_in & 1);
    int hcarry = hsum >> 4;
    uint64_t usum = (x & 0xff) + (y & 0xff) + (carry_in & 1);
    // int64_t ssum = (int64_t)(signed char)x + (int64_t)(signed char)y + (int64_t)(signed char)carry_in;
    int carry_out = usum != (uint8_t)usum;
    int overflow = ((usum ^ x) & (usum ^ y)) >> 7;
    
    uint8_t result = usum;
    int zero = !result;
    int neg = result >> 7;

    cpu->f_z = zero;
    cpu->f_pv = overflow;  //might need to be !overflow
    cpu->f_c = carry_out;
    cpu->f_s = neg;
    cpu->f_h = hcarry;
    // cpu->f_n   set by caller


    // uint64_t hsum = (x & 0xf) + (y & 0xf) + carry_in;
    // int hcarry = hsum >> 4;
    // uint64_t usum = x + y + carry_in;
    // int64_t ssum = (int64_t)(signed char)x + (int64_t)(signed char)y + (int64_t)(signed char)carry_in;
    // int carry_out = usum != (uint8_t)usum;
    // int overflow = ssum != (int8_t)ssum;
    
    // uint8_t result = usum;
    // int zero = !result;
    // int neg = result >> 7;

    // cpu->f_z = zero;
    // cpu->f_pv = overflow;  //might need to be !overflow
    // cpu->f_c = carry_out;
    // cpu->f_s = neg;
    // cpu->f_h = hcarry;
    // // cpu->f_n   set by caller


    return result;
}

static unsigned add_8(struct cpu *cpu, unsigned x, unsigned y){
    return alu_8_add(cpu, x, y, 0);
    cpu->f_n = 0;
}

static unsigned sub_8(struct cpu *cpu, unsigned x){
    // return alu_8_add(cpu, x, ~y, 1);
    // cpu->f_n = 1;

    unsigned long rflags = cpu->f_pv<<11 | (cpu->f & 0xd1);
    unsigned long src = x;
    unsigned long src_dst = cpu->a;

    __asm__ __volatile__ (
        "pushfq\n\t"

        "pushfq\n\t"
        "andw $0xf72a,0(%%rsp)\n\t"
        "orw %w0,0(%%rsp)\n\t"
        "popfq\n\t"

        "subb %b2, %b1\n\t"

        "pushfq\n\t"
        "popq %0\n\t"

        "popfq\n\t"

        :"+&q"(rflags),"+&q"(src_dst)
        :"q"(src)
    );

    cpu->f_c  = (rflags>>0)&1;
    cpu->f_n  = 1;
    cpu->f_pv = (rflags>>11)&1;
    cpu->f_h  = (rflags>>4)&1;
    cpu->f_z  = (rflags>>6)&1;
    cpu->f_s  = (rflags>>7)&1;

    return src_dst;
}

static void inc_8(struct cpu *cpu, unsigned char *p){
    unsigned char c = cpu->f_c;
    *p = alu_8_add(cpu, *p, 1, 0);
    cpu->f_n = 0;
    cpu->f_c = c;
}

static void dec_8(struct cpu *cpu, unsigned char *p){
    unsigned char c = cpu->f_c;
    *p = alu_8_add(cpu, *p, (unsigned char)~1, 1); // litte scary
    cpu->f_n = 1;
    cpu->f_c = c;
}

static void cp_8(struct cpu *cpu, unsigned char b){
//    alu_8_add(cpu, cpu->a, ~b, 1);
//    cpu->f_n = 1;

    unsigned long rflags = cpu->f_pv<<11 | (cpu->f & 0xd1);
    unsigned long src = b;
    unsigned long src_dst = cpu->a;

    __asm__ __volatile__ (
        "pushfq\n\t"

        "pushfq\n\t"
        "andw $0xf72a,0(%%rsp)\n\t"
        "orw %w0,0(%%rsp)\n\t"
        "popfq\n\t"

        "cmpb %b2, %b1\n\t"

        "pushfq\n\t"
        "popq %0\n\t"

        "popfq\n\t"

        :"+&q"(rflags),"+&q"(src_dst)
        :"q"(src)
    );

    cpu->f_c  = (rflags>>0)&1;
    cpu->f_n  = 1;
    cpu->f_pv = (rflags>>11)&1;
    cpu->f_h  = (rflags>>4)&1;
    cpu->f_z  = (rflags>>6)&1;
    cpu->f_s  = (rflags>>7)&1;
}

static void neg_8(struct cpu *cpu, unsigned char *byte1){
    // *byte1 = alu_8_add(cpu, 0, ~*byte1, 1);
    // cpu->f_n = 1;

    unsigned long rflags = cpu->f_pv<<11 | (cpu->f & 0xd1);
    unsigned long src_dst = *byte1;

    __asm__ __volatile__ (
        "pushfq\n\t"

        "pushfq\n\t"
        "andw $0xf72a,0(%%rsp)\n\t"
        "orw %w0,0(%%rsp)\n\t"
        "popfq\n\t"

        "negb %b1\n\t"

        "pushfq\n\t"
        "popq %0\n\t"

        "popfq\n\t"

        :"+&q"(rflags),"+&q"(src_dst)
    );

    *byte1 = src_dst;

    cpu->f_c  = (rflags>>0)&1;
    cpu->f_n  = 1;
    cpu->f_pv = (rflags>>11)&1;
    cpu->f_h  = (rflags>>4)&1;
    cpu->f_z  = (rflags>>6)&1;
    cpu->f_s  = (rflags>>7)&1;
}

// should change to be consistent with sbc_8
static void adc_8(struct cpu *cpu, unsigned char *src_dst_ptr, unsigned char src){
    *src_dst_ptr = alu_8_add(cpu, *src_dst_ptr, src, cpu->f_c);
    cpu->f_n = 0;
    //adc_8(cpu, &cpu->a, ram[cpu->hl]);
}

static unsigned sbc_8(struct cpu *cpu, unsigned x){
    // return alu_8_add(cpu, x, ~y, 1);
    // cpu->f_n = 1;

#if 1
    unsigned long rflags = cpu->f_pv<<11 | (cpu->f & 0xd1);
    unsigned long src = x;
    unsigned long src_dst = cpu->a;

    __asm__ __volatile__ (
        "pushfq\n\t"

        "pushfq\n\t"
        "andw $0xf72a,0(%%rsp)\n\t"
        "orw %w0,0(%%rsp)\n\t"
        "popfq\n\t"

        "sbbb %b2, %b1\n\t"

        "pushfq\n\t"
        "popq %0\n\t"

        "popfq\n\t"

        :"+&q"(rflags),"+&q"(src_dst)
        :"q"(src)
    );

    cpu->f_c  = (rflags>>0)&1;
    cpu->f_n  = 1;
    cpu->f_pv = (rflags>>11)&1;
    cpu->f_h  = (rflags>>4)&1;
    cpu->f_z  = (rflags>>6)&1;
    cpu->f_s  = (rflags>>7)&1;

    return src_dst;
#endif
#if 0
    unsigned tmp = cpu->a - x - cpu->f_c;
    cpu->f_c    = tmp >> 8;
    cpu->f_s    = tmp >> 7;
    cpu->f_pv   = (cpu->a ^ x ^ cpu->f_c) >> 7; //might be wrong
    cpu->f_n    = 1;
    cpu->f_z = !(unsigned char)tmp;
    return (unsigned char)tmp;
#endif

}

static unsigned add16(struct cpu *cpu, unsigned x, unsigned y, unsigned carry_in){
    uint64_t hsum = (x & 0xfff) + (y & 0xfff) + carry_in;
    int hcarry = hsum >> 12;
    uint64_t usum = (x & 0xffff) + (y & 0xffff) + carry_in;
    // int64_t ssum = (int64_t)(signed short)x + (int64_t)(signed short)y + (int64_t)(signed short)carry_in;
    unsigned carry_out = usum != (uint16_t)usum;
    // int overflow = ssum != (int8_t)ssum;
    
    uint16_t result = usum;
    // int zero = !result;
    // int neg = result >> 15;

    // cpu->f_z = zero;
    // cpu->f_pv = overflow;  //might need to be !overflow
    cpu->f_c = carry_out;
    // cpu->f_s = neg;
    cpu->f_h = hcarry;
    // cpu->f_n   set by caller



    // uint64_t hsum = (x & 0xfff) + (y & 0xfff) + carry_in;
    // int hcarry = hsum >> 12;
    // uint64_t usum = x + y + carry_in;
    // // int64_t ssum = (int64_t)(signed short)x + (int64_t)(signed short)y + (int64_t)(signed short)carry_in;
    // unsigned carry_out = usum != (uint16_t)usum;
    // // int overflow = ssum != (int8_t)ssum;
    
    // uint16_t result = usum;
    // // int zero = !result;
    // // int neg = result >> 15;

    // // cpu->f_z = zero;
    // // cpu->f_pv = overflow;  //might need to be !overflow
    // cpu->f_c = carry_out;
    // // cpu->f_s = neg;
    // cpu->f_h = hcarry;
    // // cpu->f_n   set by caller



    return result;
}

static void sbc_16(struct cpu *cpu, unsigned short *pshort1, unsigned short *pshort2){
    // Could be bug here, one of the next 4 lines is probably correct, don't know which
//    *pshort1 = add16(cpu, *pshort1, ~*pshort2, 1);
  //  *pshort1 = add16(cpu, *pshort1, ~cpu->f_c, 1);
    //cpu->f_n = 1;


    unsigned long rflags = cpu->f_pv<<11 | (cpu->f & 0xd1);
    unsigned long src = *pshort2;
    unsigned long src_dst = *pshort1;

    __asm__ __volatile__ (
        "pushfq\n\t"

        "pushfq\n\t"
        "andw $0xf72a,0(%%rsp)\n\t"
        "orw %w0,0(%%rsp)\n\t"
        "popfq\n\t"

        "sbbw %w2, %w1\n\t"

        "pushfq\n\t"
        "popq %0\n\t"

        "popfq\n\t"

        :"+&q"(rflags),"+&q"(src_dst)
        :"q"(src)
    );

    *pshort1 = src_dst;

    cpu->f_c  = (rflags>>0)&1;
    cpu->f_n  = 1;
    cpu->f_pv = (rflags>>11)&1;
    cpu->f_h  = (rflags>>4)&1;
    cpu->f_z  = (rflags>>6)&1;
    cpu->f_s  = (rflags>>7)&1;
}

static void or_8(struct cpu *cpu, unsigned char val){
    cpu->a |= val;
    cpu->f_c = 0;
    cpu->f_n = 0;
    cpu->f_pv = parity(cpu->a);
    cpu->f_h = 0;
    cpu->f_z = !cpu->a;
    cpu->f_s = cpu->a >> 7; // take sign bit and put it in f_s
}

static void and_8(struct cpu *cpu, unsigned char val){
    cpu->a &= val;
    cpu->f_c = 0;
    cpu->f_n = 0;
    cpu->f_pv = parity(cpu->a);
    cpu->f_h = 1;
    cpu->f_z = !cpu->a;
    cpu->f_s = cpu->a >> 7; // take sign bit and put it in f_s
}

static unsigned short *writers;
static unsigned char *mem_tracker;


static unsigned char load_8(struct cpu *restrict const cpu, const unsigned char *restrict const ram, unsigned short addr){
    (void)cpu; // I like handing cpu even though I am not using it
    unsigned char byte1 = ram[addr];
    mem_tracker[addr] |= 0x01;
    return byte1;
}

static void store_8(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned char val, unsigned short addr){
    (void)cpu; // I like handing cpu even though I am not using it
    ram[addr] = val; // write low bits

    mem_tracker[addr] |= 0x02;
    writers[addr] = cpu->pc;
}

static unsigned char imm_8(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    unsigned short addr = cpu->pc++;
    unsigned char low = ram[addr];
    mem_tracker[addr] |= 0x04;

    if(mem_tracker[addr] != 0x04){
        printf("Detected bad stuff, address %04hx last written by %04hx\n", addr, writers[addr]);
        exit(44);
    }

    return low;
}

/*
static void push_8(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned char val){
    cpu->sp--;
    store_8(cpu, ram, val, cpu->sp); // push low bits
}
*/
/*
static unsigned char pop_8(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    return load_8(cpu, ram,cpu->sp++);
}
*/

static unsigned short load_16(struct cpu *restrict const cpu, const unsigned char *restrict const ram, unsigned short addr){
    (void)cpu; // I like handing cpu even though I am not using it
    unsigned char byte1 = load_8(cpu, ram, addr++);
    unsigned char byte2 = load_8(cpu, ram, addr);

    return byte1 | (byte2 << 8);
}

static void store_16(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned short val, unsigned short addr){
    (void)cpu; // I like handing cpu even though I am not using it
    store_8(cpu, ram, val, addr);                            // write low bits
    store_8(cpu, ram, val >> 8, (unsigned short)(addr + 1)); // write high bits
}

static unsigned short pop_16(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    unsigned short tmp = load_16(cpu, ram, cpu->sp);
    cpu->sp += 2;

    return tmp;
}

static void push_16(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned short val){
    cpu->sp -= 2;
    store_16(cpu, ram, val, cpu->sp);
}

static unsigned short imm_16(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    unsigned char low = imm_8(cpu, ram);
    unsigned char high = imm_8(cpu, ram);

    return high << 8 | low;
}

static unsigned short bdos(
    unsigned char *restrict ram, 
    unsigned char function, 
    unsigned short parameter
){
    unsigned char tmp_byte;
    switch (function){
    case 0x19: // return currently selected drive
        return 0; // drive A:
    case 0x0f: // open a file
        printf("BDOS open %04hx  %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx   %02hhx %02hhx %02hhx  \"%-8.8s.%-3.3s\"\n",
            parameter,
            ram[parameter+1],
            ram[parameter+2],
            ram[parameter+3],
            ram[parameter+4],
            ram[parameter+5],
            ram[parameter+6],
            ram[parameter+7],
            ram[parameter+8],

            ram[parameter+9],
            ram[parameter+10],
            ram[parameter+11],

            ram+parameter+1,
            ram+parameter+9
        );
        //exit(2);
        return 0xff;
        break;
    case 0x23: // get file size or something
        return 0xff;
        break;
    case 0x00: // System Reset, exit
        puts("Good Bye");
        exit(0);
    case 0x06: // Direct Console I/O
        tmp_byte = parameter & 0xff;
        if(tmp_byte == 0xff){
            printf("\n\n\nTHEY WANT INPUT\n\n\n");
            exit(2);
            return 0x00; // no char ready
        }else{
            putchar(parameter & 0xff);
            fflush(stdout);
            return 0x00; // might be wrong
        }
    default:
        printf("Function: %02hhx, parameter: %04hx\n", function, parameter);
        exit(2);
    }
}

static void do_emulation(struct cpu *cpu, unsigned char *restrict ram){
    FILE *fp = fopen("debug.txt", "wb");
    unsigned long long ran = 0;
    unsigned short oldoldoldpc = 0xffff;
    unsigned short oldoldpc = 0xffff;
    unsigned short oldpc = 0xffff;

    {
        FILE *ram_file = fopen("/home/man/Desktop/Dev/ref_z80_emu/tnylpo/ram_at_start.bin", "rb");
        if(!ram_file)
            exit(45);

        fread(ram, 0x10000, 1, ram_file);
        fclose(ram_file);
    }

    for(;;){
        ran++;

        // printf("Bytes %02hhx %02hhx %02hhx %02hhx at 0x%04hx after %llu run\n",
        //     ram[cpu->pc],
        //     ram[cpu->pc+1],
        //     ram[cpu->pc+2],
        //     ram[cpu->pc+3],
        //     cpu->pc,
        //     ran++
        // );
        
        ////////////////////////////////////////////////////////////////////////////        
        cpu->f &= 0x41;  // only Z and C matter for gorillas
#if 1
        const unsigned long print_start = 0x19f80;
        const unsigned long prints_wanted = 2000;


        if(ran > print_start){ 
            fprintf(fp, "%016llx Bytes %02hhx %02hhx %02hhx %02hhx pc:%04hx af:%04hx sp:%04hx hl:%04hx de:%04hx bc:%04hx ix:%04hx iy:%04hx\n",
                ran,
                ram[cpu->pc],
                ram[cpu->pc+1],
                ram[cpu->pc+2],
                ram[cpu->pc+3],
                cpu->pc,
                cpu->af& 0xffd7 & 0xffef, // hide reserved bits and half carry
                cpu->sp, 
                cpu->hl, 
                cpu->de,
                cpu->bc, 
                cpu->ix, 
                cpu->iy
            );
            fflush(fp);
            if(ran > print_start + prints_wanted)
                exit(0);
        }
#endif

        // fprintf(stdout, "Bytes %02hhx %02hhx %02hhx %02hhx pc:%04hx af:%04hx sp:%04hx hl:%04hx de:%04hx bc:%04hx ix:%04hx iy:%04hx\n",
        //     ram[cpu->pc],
        //     ram[cpu->pc+1],
        //     ram[cpu->pc+2],
        //     ram[cpu->pc+3],
        //     cpu->pc,
        //     cpu->af, 
        //     cpu->sp, 
        //     cpu->hl, 
        //     cpu->de,
        //     cpu->bc, 
        //     cpu->ix, 
        //     cpu->iy
        // );

        oldoldoldpc = oldoldpc;
        oldoldpc = oldpc;
        oldpc = cpu->pc;

        if(cpu->pc < 0x0100 || cpu->pc > 30300){ // tmp debug code
            puts("highly suspect PC value");
            goto fail;
        }

        unsigned char opcode = imm_8(cpu, ram);

        unsigned char byte1;
        unsigned char byte2;
        unsigned char tmp_uchar; (void)tmp_uchar;
        unsigned short tmp_ushort;
        unsigned char *ptr_u8;


        switch (opcode){
        case 0xc3: // jp **
            cpu->pc = imm_16(cpu, ram);
            break;
        case 0x3e: // ld a,*
            byte1 = imm_8(cpu, ram);
            cpu->a = byte1;
            break;
        case 0x32: // ld (**), a
            store_8(cpu, ram, cpu->a, imm_16(cpu, ram));
            break;
        case 0x2a: // ld hl, (**)
            cpu->hl = load_16(cpu, ram, imm_16(cpu, ram));
            break;
        case 0xed: // Extended Instructions
            switch(imm_8(cpu, ram)){
            case 0x7b: // ld sp, (**)
                cpu->sp = load_16(cpu, ram, imm_16(cpu, ram));
                break;
            case 0xb0: // ldir
                //basically a memcpy
                //do store_8(cpu, ram, load_8(cpu, ram, cpu->hl++), cpu->de++);
                //while ((unsigned short)--cpu->bc);
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_pv = !!(cpu->bc - 1);
                store_8(cpu, ram, load_8(cpu, ram, cpu->hl++), cpu->de++);
                if((unsigned short)--cpu->bc)
                    cpu->pc = oldpc;
                break;
            case 0x42: // sbc hl,bc
                sbc_16(cpu, &cpu->hl, &cpu->bc);
                break;
            case 0x57: // some instruction I can skip doing
                break;
            case 0x43: // ld (**),bc
                store_16(cpu, ram, cpu->bc, imm_16(cpu, ram));
                break;
            case 0x53: // ld (**),de
                store_16(cpu, ram, cpu->de, imm_16(cpu, ram));
                break;
            case 0x52: // sbc hl,de
                sbc_16(cpu, &cpu->hl, &cpu->de);
                break;
            case 0x5b: // ld de,(**)
                cpu->de = load_16(cpu, ram, imm_16(cpu, ram));
                break;
            case 0x4b: // ld bc,(**)
                cpu->bc = load_16(cpu, ram, imm_16(cpu, ram));
                break;
            case 0x44: // neg
                // cpu->a = add8(cpu, 0, ~cpu->a, 1);
                // cpu->f_n = 1;
                neg_8(cpu, &cpu->a);
                break;
            case 0x6a: // adc hl,hl
                cpu->hl = add16(cpu, cpu->hl, cpu->hl, cpu->f_c);
                cpu->f_n = 0;
                break;
            case 0xb8: // lddr
                // do store_8(cpu, ram, load_8(cpu, ram, cpu->hl--), cpu->de--);
                // while ((unsigned short)--cpu->bc);
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_pv = !!(cpu->bc - 1);
                store_8(cpu, ram, load_8(cpu, ram, cpu->hl--), cpu->de--);
                if((unsigned short)--cpu->bc)
                    cpu->pc = oldpc;

                break;
            default:
                puts("0xed means Extended Instruction");
                goto fail;
            }
            break;
        case 0x2b: // dec hl
            cpu->hl--;
            break;
        case 0x56: // ld d, (hl)
            cpu->d = load_8(cpu, ram, cpu->hl);
            break;
        case 0x5e: // ld e, (hl)
            cpu->e = load_8(cpu, ram, cpu->hl);
            break;
        case 0xeb: // ex de, hl
            tmp_ushort = cpu->de;
            cpu->de = cpu->hl;
            cpu->hl = tmp_ushort;
            break;
        case 0x23: // inc hl
            cpu->hl++;
            break;
        case 0x19: // add hl, de
            //cpu->f_h = ((cpu->hl & 0x0fff) + (cpu->de & 0x0fff)) >> 12;
            //cpu->hl += cpu->de;

            cpu->hl = add16(cpu, cpu->hl, cpu->de, 0);
            cpu->f_n = 0;
            break;
        case 0xd5: // push de
            push_16(cpu, ram, cpu->de);
            break;
        case 0x01: // ld bc, **
            cpu->bc = imm_16(cpu, ram);
            break;
        case 0xfd: // IY Instructions
            switch(imm_8(cpu, ram)){
            case 0x21: // ld iy, **
                cpu->iy = imm_16(cpu, ram);
                break;
            case 0xe9: // jp (iy) ...the syntex of this instruction is off
                cpu->pc = cpu->iy;
                break;
            case 0xe5: // push iy
                push_16(cpu, ram, cpu->iy);
                break;
            case 0xe1: // pop iy
                cpu->iy = pop_16(cpu, ram);
                break;
            case 0x2a: // ld iy,(**)
                cpu->iy = load_16(cpu, ram, imm_16(cpu, ram));
                break;
            default:
                puts("0xfd is an IY instruction");
                goto fail;
            }
            break;
        case 0x1a: // ld a, (de)
            cpu->a = load_8(cpu, ram, cpu->de);
            break;
        case 0x13: // inc de
            cpu->de++;
            break;
        case 0xfe: // cp *     probably should be something like `cp a,*` or `cp *,a`
            // page 164 in z80 cpu manual
            byte1 = imm_8(cpu, ram);
            cp_8(cpu, byte1);
            break;
        case 0xca: // jp z,**
            tmp_ushort = imm_16(cpu, ram);
            if (cpu->f_z)
                cpu->pc = tmp_ushort;
            break;
        case 0xdd: // IX Instructions
            switch(imm_8(cpu, ram)){
            case 0xe5: // push ix
                push_16(cpu, ram, cpu->ix);
                break;
            case 0x21: // ld ix,**
                cpu->ix = imm_16(cpu, ram);
                break;
            case 0x39: // add ix,sp
                cpu->ix = add16(cpu, cpu->ix, cpu->sp, 0);
                cpu->f_n = 0;
                break;
            case 0xe1: // pop ix
                cpu->ix = pop_16(cpu, ram);
                break;
            case 0x6e: // ld l,(ix+*)
                cpu->l = load_8(cpu, ram, cpu->ix + (signed char)imm_8(cpu, ram));
                break;
            case 0x66: // ld h,(ix+*)
                cpu->h = load_8(cpu, ram, cpu->ix + (signed char)imm_8(cpu, ram));
                break;
            case 0xf9: // ld sp,ix
                cpu->sp = cpu->ix;
                break;
            case 0x22: // ld (**), ix
                store_16(cpu, ram, cpu->ix, imm_16(cpu, ram));
                break;
            case 0x2a: // ld ix,(**)
                cpu->ix = load_16(cpu, ram, imm_16(cpu, ram));
                break;
            default:
                puts("0xdd means an IX instruction");
                goto fail;
            }
            break;
        case 0xc5: // push bc
            push_16(cpu, ram, cpu->bc);
            break;
        case 0x6f: // ld l,a
            cpu->l = cpu->a;
            break;
        case 0x26: // ld h,*
            byte1 = imm_8(cpu, ram);
            cpu->h = byte1;
            break;
        case 0x39: // add hl,sp
            cpu->hl = add16(cpu, cpu->hl, cpu->sp, 0);
            cpu->f_n = 0;
            break;
        case 0x3a: // ld a,(**)
            cpu->a = load_8(cpu, ram, imm_16(cpu, ram));
            break;
        case 0xbc: // cp h
            cp_8(cpu, cpu->h);
            break;
        case 0x30: // jr nc,*
            byte1 = imm_8(cpu, ram);
            if(!cpu->f_c)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x46: // ld b,(hl)
            cpu->b = load_8(cpu, ram, cpu->hl);
            break;
        case 0x24: // inc h
            // byte2 = cpu->f_c;
            // cpu->h = add8(cpu, cpu->h, 1, 0);
            // cpu->f_c = byte2;
            inc_8(cpu, &cpu->h);
            break;
        case 0x66: // ld h,(hl)
            cpu->h = load_8(cpu, ram, cpu->hl);
            break;
        case 0x68: // ld l, b
            cpu->l = cpu->b;
            break;
        case 0xe9: // jp (hl)
            cpu->pc = cpu->hl;
            break;
        case 0xd9: // exx
            tmp_ushort = cpu->bc;
            cpu->bc = cpu->bc_prime;
            cpu->bc_prime = tmp_ushort;

            tmp_ushort = cpu->de;
            cpu->de = cpu->de_prime;
            cpu->de_prime = tmp_ushort;

            tmp_ushort = cpu->hl;
            cpu->hl = cpu->hl_prime;
            cpu->hl_prime = tmp_ushort;

            break;
        case 0xaf: // xor a
            cpu->a ^= cpu->a;
            cpu->f_c = 0;
            cpu->f_n = 0;
            cpu->f_pv = parity(cpu->a);
            cpu->f_h = 0;
            cpu->f_z = !cpu->a;
            cpu->f_s = cpu->a >> 7; // take sign bit and put it in f_s

            break;
        case 0x67: // ld h,a
            cpu->h = cpu->a;
            break;
        case 0xe5: //push hl
            push_16(cpu, ram, cpu->hl);
            break;
        case 0x21: // ld hl,**
            cpu->hl = imm_16(cpu, ram);
            break;
        case 0xcd: // call **
            tmp_ushort = imm_16(cpu, ram);
            push_16(cpu, ram, cpu->pc);
            cpu->pc = tmp_ushort;

            if(cpu->pc == 5){
                cpu->hl = bdos(ram, cpu->c, cpu->de);
                cpu->a = cpu->l;
                cpu->b = cpu->h;
                cpu->pc = pop_16(cpu, ram); // Undo the push_16 above
            }

            break;
        case 0xf3: // di
            // printf("Interrupts off, di instruction not written\n");
            // fprintf(fp, "Interrupts off, di instruction not written\n");
            break;
        case 0x22: // ld (**), hl
            store_16(cpu, ram, cpu->hl, imm_16(cpu, ram));
            break;
        case 0xe1: // pop hl
            cpu->hl = pop_16(cpu, ram);
            break;
        case 0xe3: // ex (sp),hl
            tmp_ushort = cpu->hl;
            cpu->hl = load_16(cpu, ram, cpu->sp);
            store_16(cpu, ram, tmp_ushort, cpu->sp);
            break;
        case 0xf5: // push af
            push_16(cpu, ram, cpu->af);
            break;
        case 0x08: // ex af,af'
            tmp_ushort = cpu->af;
            cpu->af = cpu->af_prime;
            cpu->af_prime = tmp_ushort;
            break;
        case 0x4d: // ld c,l
            cpu->c = cpu->l;
            break;
        case 0x44: // ld b,h
            cpu->b = cpu->h;
            break;
        case 0xf9: // ld sp,hl
            cpu->sp = cpu->hl;
            break;
        case 0x7d: // ld a,l
            cpu->a = cpu->l;
            break;
        case 0x02: // ld (bc),a
            store_8(cpu, ram, cpu->a, cpu->bc);
            break;
        case 0x03: // inc bc
            cpu->bc++;
            break;
        case 0x7c: // ld a,h
            cpu->a = cpu->h;
            break;
        case 0xd1: // pop de
            cpu->de = pop_16(cpu, ram);
            break;
        case 0x7e: // ld a,(hl)
            cpu->a = load_8(cpu, ram, cpu->hl);
            break;
        case 0xb4: // or h
            or_8(cpu, cpu->h);
            break;
        case 0x4f: // ld c,a
            cpu->c = cpu->a;
            break;
        case 0x47: // ld b,a
            cpu->b = cpu->a;
            break;
        case 0xc1: // pop bc
            cpu->bc = pop_16(cpu, ram);
            break;
        case 0xb5: // or l
            or_8(cpu, cpu->l);
            break;
        case 0x28: // jr z,*
            byte1 = imm_8(cpu, ram);
            if(cpu->f_z)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x09: // add hl,bc
            cpu->hl = add16(cpu, cpu->hl, cpu->bc, 0);
            cpu->f_n = 0;
            break;
        case 0x4e: // ld c,(hl)
            cpu->c = load_8(cpu, ram, cpu->hl);
            break;
        case 0x06: // ld b,*
            byte1 = imm_8(cpu, ram);
            cpu->b = byte1;
            break;
        case 0x18: // jr *
            byte1 = imm_8(cpu, ram);
            cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0xb7: // or a
            or_8(cpu, cpu->a);
            break;
        case 0xf1: // pop af
            cpu->af = pop_16(cpu, ram);
            break;
        case 0xfb: // ei
            break;
        case 0xea: // jp pe, **
            tmp_ushort = imm_16(cpu, ram);
            if (cpu->f_pv)
                cpu->pc = tmp_ushort;
            break;
        case 0xe6: // and *
            byte1 = imm_8(cpu, ram);
            and_8(cpu, byte1);
            break;
        case 0x87: // add a,a
            cpu->a = add_8(cpu, cpu->a, cpu->a);
            break;
        case 0xc2: // jp nz,**
            tmp_ushort = imm_16(cpu, ram);
            if (!cpu->f_z)
                cpu->pc = tmp_ushort;
            break;
        case 0x71: // ld (hl),c
            store_8(cpu, ram, cpu->c, cpu->hl);
            break;
        case 0x70: // ld (hl),b
            store_8(cpu, ram, cpu->b, cpu->hl);
            break;
        case 0x73: // ld (hl),e
            store_8(cpu, ram, cpu->e, cpu->hl);
            break;
        case 0x07: // rlca
            cpu->a = cpu->a << 1 | cpu->a >> 7;
            cpu->f_c = cpu->a & 1;
            break;
        case 0xcb:
            byte1 = imm_8(cpu, ram);
            
            switch (byte1 & 0x07){
            case 0:
                ptr_u8 = &cpu->b;
                break;
            case 1:
                ptr_u8 = &cpu->c;
                break;
            case 2:
                ptr_u8 = &cpu->d;
                break;
            case 3:
                ptr_u8 = &cpu->e;
                break;
            case 4:
                ptr_u8 = &cpu->h;
                break;
            case 5:
                ptr_u8 = &cpu->l;
                break;
            case 6:
                ptr_u8 = cpu->hl + ram;
                break;
            case 7:
                ptr_u8 = &cpu->a;
                break;
            default:
                __builtin_unreachable();
                break;
            }

            switch (byte1 >> 6){
            case 0:
                switch (byte1 >> 3 & 0x07){
                case 0: // rlc
                    byte2 = *ptr_u8;
                    *ptr_u8 = *ptr_u8 << 1 | *ptr_u8 >> 7;
                    cpu->f_c = byte2 >> 7;
                    cpu->f_n = 0;
                    cpu->f_h = 0;
                    cpu->f_z = !*ptr_u8;
                    cpu->f_s = *ptr_u8 >> 7;
                    cpu->f_pv = parity(*ptr_u8);
                    break;
                case 1: // rrc
                    cpu->f_c = *ptr_u8 & 1;
                    *ptr_u8  = *ptr_u8 >> 1 | *ptr_u8 << 7;
                    cpu->f_n = 0;
                    cpu->f_h = 0;
                    cpu->f_z = !*ptr_u8;
                    cpu->f_s = *ptr_u8 >> 7;
                    cpu->f_pv = parity(*ptr_u8);
                    break;
                case 2: // rl
                    byte2 = *ptr_u8;
                    *ptr_u8 = *ptr_u8 << 1 | cpu->f_c;
                    cpu->f_c = byte2 >> 7;
                    cpu->f_n = 0;
                    cpu->f_h = 0;
                    cpu->f_z = !*ptr_u8;
                    cpu->f_s = *ptr_u8 >> 7;
                    cpu->f_pv = parity(*ptr_u8);
                    break;
                case 3: // rr
                    byte2 = *ptr_u8;
                    *ptr_u8  = *ptr_u8 >> 1 | cpu->f_c << 7;
                    cpu->f_c = byte2 & 1;
                    cpu->f_n = 0;
                    cpu->f_h = 0;
                    cpu->f_z = !*ptr_u8;
                    cpu->f_s = *ptr_u8 >> 7;
                    cpu->f_pv = parity(*ptr_u8);

                    break;
                case 4: // sla
                case 6: // sll ( undocumented )
                    cpu->f_c = *ptr_u8 >> 7 & 1;
                    *ptr_u8 <<= 1;
                    cpu->f_n = 0;
                    cpu->f_h = 0;
                    cpu->f_z = !*ptr_u8;
                    cpu->f_s = *ptr_u8 >> 7;
                    cpu->f_pv = parity(*ptr_u8);
                    break;
                case 5: // sra
                    puts("asdasd");
                    exit(2);
                    break;
                case 7: // srl
                    cpu->f_c = *ptr_u8 & 1;
                    *ptr_u8 >>= 1;
                    cpu->f_n = 0;
                    cpu->f_h = 0;
                    cpu->f_z = !*ptr_u8;
                    cpu->f_s = *ptr_u8 >> 7;
                    cpu->f_pv = parity(*ptr_u8);
                    break;
                default:
                    __builtin_unreachable();
                    break;
                }
                break;
            case 1:
                cpu->f_z = ~*ptr_u8 >> (byte1 >> 3 & 0x07);
                cpu->f_h = 1;
                cpu->f_n = 0;
                break;
            case 2:
                *ptr_u8 &= ~(1 << (byte1 >> 3 & 0x07));
                break;
            case 3:
                *ptr_u8 |= 1 << (byte1 >> 3 & 0x07);
                break;
            
            default:
                __builtin_unreachable();
                break;
            }
            break;
        case 0x3d: // dec a
            //byte2 = cpu->f_c;
            //cpu->a = add8(cpu, cpu->a, (unsigned char)~1, 1);
            //cpu->f_c = byte2;
            dec_8(cpu, &cpu->a);
            break;
        case 0x20: // jr nz,*
            byte1 = imm_8(cpu, ram);
            if(!cpu->f_z)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x69: // ld l,c    program will not work without this instruction
            cpu->l = cpu->c;
            break;
        case 0x60: // ld h,b
            cpu->h = cpu->b;
            break;
        case 0x37: // scf
            cpu->f_n = 0;
            cpu->f_h = 0;
            cpu->f_c = 1;
            break;
        case 0xc9: // ret
            cpu->pc = pop_16(cpu,ram);
            break;
        case 0xd8: // ret c
            if (cpu->f_c)
                cpu->pc = pop_16(cpu,ram);
            break;
        case 0x7a: // ld a,d
            cpu->a = cpu->d;
            break;
        case 0xb3: // or e
            or_8(cpu, cpu->e);
            break;
        case 0x38: // jr c,*
            byte1 = imm_8(cpu, ram);
            if(cpu->f_c)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x77: // ld (hl),a
            store_8(cpu, ram, cpu->a, cpu->hl);
            break;
        case 0x11: // ld de,**
            cpu->de = imm_16(cpu, ram);
            break;
        case 0x12: // ld (de),a
            store_8(cpu, ram, cpu->a, cpu->de);
            break;
        case 0x5d: // ld e,l
            cpu->e = cpu->l;
            break;
        case 0x54: // ld d,h
            cpu->d = cpu->h;
            break;
        case 0x0b: // dec bc
            cpu->bc--;
            break;
        case 0x36: // ld (hl),*
            store_8(cpu, ram, imm_8(cpu, ram), cpu->hl);
            break;
        case 0x5f: // ld e,a
            cpu->e = cpu->a;
            break;
        case 0x6e: // ld l,(hl)
            cpu->l = load_8(cpu, ram, cpu->hl);
            break;
        case 0x16: // ld d,*
            cpu->d = imm_8(cpu, ram);
            break;
        case 0x1c: // inc e
            inc_8(cpu, &cpu->e);
            break;
        case 0x1d: // dec e
            dec_8(cpu, &cpu->e);
            break;
        case 0x78: // ld a,b
            cpu->a = cpu->b;
            break;
        case 0xb1: // or c
            or_8(cpu, cpu->c);
            break;
        case 0x57: // ld d,a
            cpu->d = cpu->a;
            break;
        case 0x8e: // adc a,(hl)
            // cpu->a = add8(cpu, cpu->a, load_16(cpu, ram, cpu->hl), cpu->f_c);
            // cpu->f_n = 0;
            adc_8(cpu, &cpu->a, load_8(cpu, ram, cpu->hl));
            break;
        case 0x04: // inc b
            // byte2 = cpu->f_c;
            // cpu->b = add8(cpu, cpu->b, 1, 0);
            // cpu->f_c = byte2;
            inc_8(cpu, &cpu->b);
            break;
        case 0xd2: // jp nc,**
            tmp_ushort = imm_16(cpu, ram);
            if (!cpu->f_c)
                cpu->pc = tmp_ushort;
            break;
        case 0x72: // ld (hl),d
            store_8(cpu, ram, cpu->d, cpu->hl);
            break;
        case 0x1f: // rra
            tmp_uchar = cpu->f_c;
            cpu->f_c = cpu->a;
            cpu->a = cpu->a >> 1 | tmp_uchar << 7;
            cpu->f_n = 0;
            cpu->f_h = 0;
            break;
        case 0xdc: // call c,**
            tmp_ushort = imm_16(cpu, ram);
            if(cpu->f_c){
                push_16(cpu, ram, cpu->pc);
                cpu->pc = tmp_ushort;

                if(cpu->pc == 5){
                    cpu->hl = bdos(ram, cpu->c, cpu->de);
                    cpu->a = cpu->l;
                    cpu->b = cpu->h;
                    cpu->pc = pop_16(cpu, ram); // Undo the push_16 above
                }
            }
            break;
        case 0xbd: // cp l
            cp_8(cpu, cpu->l);
            break;
        case 0x2f: // cpl
            cpu->a = ~cpu->a;
            cpu->f_n = 1;
            cpu->f_h = 1;
            break;
        case 0xd6: // sub *
            byte1 = imm_8(cpu, ram);
            cpu->a = sub_8(cpu, byte1);
            break;
        case 0x29: // add hl,hl
            cpu->hl = add16(cpu, cpu->hl, cpu->hl, 0);
            cpu->f_n = 0;
            break;
        case 0x3c: // inc a
            inc_8(cpu, &cpu->a);
            break;
        case 0x8f: // adc a,a
            adc_8(cpu, &cpu->a, cpu->a);
            break;
        case 0x14: // inc d
            inc_8(cpu, &cpu->d);
            break;
        case 0x2c: // inc l
            inc_8(cpu, &cpu->l);
            break;
        case 0xb0: // or b
            or_8(cpu, cpu->b);
            break;
        case 0xde: // sbc a,*
            byte1 = imm_8(cpu, ram);
            cpu->a = sbc_8(cpu, byte1);
            break;
        case 0xa1: // and c
            and_8(cpu, cpu->c);
            break;
        case 0xa0: // and b
            and_8(cpu, cpu->b);
            break;
        case 0x0a: // ld a,(bc)
            cpu->a = load_8(cpu, ram, cpu->bc);
            break;
        case 0x0c: // inc c
            inc_8(cpu, &cpu->c);
            break;
        case 0x0d: // dec c
            dec_8(cpu, &cpu->c);
            break;
        case 0xbe: // cp (hl)
            cp_8(cpu, load_8(cpu, ram, cpu->hl));
            break;
        case 0x05: // dec b
            dec_8(cpu, &cpu->b);
            break;
        case 0x6b: // ld l,e
            cpu->l = cpu->e;
            break;
        case 0x62: // ld h,d
            cpu->h = cpu->d;
            break;
        case 0x10: // djnz *
            byte1 = imm_8(cpu, ram);
            if(--cpu->b)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x79: // sub a
            cpu->a = sub_8(cpu, cpu->a);
            break;
        case 0xc6: // add a,*
            byte1 = imm_8(cpu, ram);
            cpu->a = add_8(cpu, byte1, cpu->a);
            break;
        default:
            puts("plain top level instruction");
fail:
            printf("Ran at %04hx %04hx %04hx\n",oldoldoldpc,oldoldpc,oldpc);
            printf("Bytes %02hhx %02hhx [%02hhx] %02hhx %02hhx %02hhx at 0x%04hx after %llu run\n",
                ram[(unsigned short)(cpu->pc-2)],
                ram[(unsigned short)(cpu->pc-1)],
                ram[cpu->pc],
                ram[cpu->pc+1],
                ram[cpu->pc+2],
                ram[cpu->pc+3],
                cpu->pc,
                ran
            );
            printf("Unknown byte %02hhx at 0x%04hx\n", opcode, oldpc);
            exit(1);
            break;
        }
    }

    fclose(fp);
}


int main(int argc, char const *argv[]) {
    (void)argc;
    //unsigned char ram[65536 + 3] = {0}; // 3 for printing opcode bytes easily
    unsigned char *ram = map_a_new_file_shared("ram.bin", 0x10000);
    memset(ram, 0x76, 0x10000); // HALT instruction
    writers = map_a_new_file_shared("writers.bin", 0x10000 * sizeof(short)); // debug stuff
    mem_tracker = map_a_new_file_shared("mem_tracker.bin", 0x10000); // debug stuff


    FILE *fp = fopen(argv[1], "rb");
    if (!fp){
        puts("No input file");
        return 1;
    }else{
        printf("got %zd bytes\n", fread(ram + 256, 1, 65536 - 256, fp));
        fclose(fp);
    }
    
    struct cpu _cpu;
    struct cpu *cpu = &_cpu;
    memset(cpu, 0, sizeof *cpu);
    cpu->pc = 256; //where CPM starts program
    ram[5] = 0xc3;
    ram[6] = 0xf5;
    ram[7] = 0xfe;
    cpu->af = 0x0000;
    cpu->sp = 0xfeed;

    // should put some fancy stuff here with a loop to deal with more then 1 guest arg
    strcpy((char *)ram + 0x82, argv[2] ? argv[2] : "");
    ram[0x81] = ' ';
    ram[0x80] = strlen((char *)ram + 0x80);

    do_emulation(cpu, ram);

    return 0;
}



/*

Here is a program to run:

https://github.com/sblendorio/gorilla-cpm

More info on CP/M here:

https://en.wikipedia.org/wiki/CP/M
http://www.classiccmp.org/cpmarchives/
http://www.cpm.z80.de/
http://www.digitalresearch.biz/CPM.HTM
http://gunkies.org/wiki/CP/M

More on Z80 CPU here:

https://en.wikipedia.org/wiki/Zilog_Z80
http://www.zilog.com/docs/z80/um0080.pdf
https://www.digikey.com/catalog/en/partgroup/z80/15507
http://z80.info/decoding.htm
https://clrhome.org/table/
https://wikiti.brandonw.net/index.php?title=Z80_Instruction_Set
https://www.ticalc.org/pub/text/z80/


*/