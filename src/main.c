#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


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

static unsigned add8(struct cpu *cpu, unsigned x, unsigned y, unsigned carry_in){

    uint64_t hsum = (x & 0xf) + (y & 0xf) + carry_in;
    int hcarry = hsum >> 4;
    uint64_t usum = x + y + carry_in;
    int64_t ssum = (int64_t)(signed char)x + (int64_t)(signed char)y + (int64_t)(signed char)carry_in;
    int carry_out = usum != (uint8_t)usum;
    int overflow = ssum != (int8_t)ssum;
    
    uint8_t result = usum;
    int zero = !result;
    int neg = result >> 7;

    cpu->f_z = zero;
    cpu->f_pv = overflow;  //might need to be !overflow
    cpu->f_c = carry_out;
    cpu->f_s = neg;
    cpu->f_h = hcarry;
    // cpu->f_n   set by caller

    return result;
}


static unsigned add16(struct cpu *cpu, unsigned x, unsigned y, unsigned carry_in){
    uint64_t hsum = (x & 0xfff) + (y & 0xfff) + carry_in;
    int hcarry = hsum >> 12;
    uint64_t usum = x + y + carry_in;
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

    return result;
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

static unsigned char pop_8(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    return ram[cpu->sp++];
}

static unsigned short pop_16(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    unsigned char low = ram[cpu->sp++];
    unsigned char high = ram[cpu->sp++];

    return high << 8 | low;
}

static void push_8(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned char val){
    cpu->sp--;
    ram[cpu->sp] = val; // push low bits
}

static void push_16(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned short val){
    cpu->sp--;
    ram[cpu->sp] = val>>8; // push high bits
    cpu->sp--;
    ram[cpu->sp] = val; // push low bits
}

static unsigned short load_16(struct cpu *restrict const cpu, const unsigned char *restrict const ram, unsigned short addr){
    (void)cpu; // I like handing cpu even though I am not using it
    unsigned char byte1 = ram[addr++];
    unsigned char byte2 = ram[addr];
    return byte1 | (byte2 << 8);
}

static void store_16(struct cpu *restrict const cpu, unsigned char *restrict const ram, unsigned short val, unsigned short addr){
    (void)cpu; // I like handing cpu even though I am not using it
    ram[addr] = val;                            // write low bits
    ram[(unsigned short)(addr + 1)] = val >> 8; // write high bits
}

static unsigned short imm_16(struct cpu *restrict const cpu, const unsigned char *restrict const ram){
    unsigned char low = ram[cpu->pc++];
    unsigned char high = ram[cpu->pc++];

    return high << 8 | low;
}

static unsigned short bdos(
    unsigned char *restrict ram, 
    unsigned char function, 
    unsigned short parameter
){
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
    default:
        printf("Function: %02hhx, parameter: %04hx\n", function, parameter);
        exit(2);
    }
}

static void do_emulation(struct cpu *cpu, unsigned char *restrict ram){
    FILE *fp = fopen("debug.txt", "wb");
    unsigned long long ran = 0;
    for(;;){
        printf("Bytes %02hhx %02hhx %02hhx %02hhx at 0x%04hx after %llu run\n",
            ram[cpu->pc],
            ram[cpu->pc+1],
            ram[cpu->pc+2],
            ram[cpu->pc+3],
            cpu->pc,
            ran++
        );
        fprintf(fp, "Bytes %02hhx %02hhx %02hhx %02hhx at 0x%04hx after %llu run...af: %04hx, sp: %04hx, hl: %04hx, de: %04hx\n",
            ram[cpu->pc],
            ram[cpu->pc+1],
            ram[cpu->pc+2],
            ram[cpu->pc+3],
            cpu->pc,ran, 
            cpu->af, 
            cpu->sp, 
            cpu->hl, 
            cpu->de
        );

        unsigned char opcode = ram[cpu->pc];
        unsigned short oldpc = cpu->pc++; // increment instruction pointer
        unsigned char byte1;
        unsigned char byte2;
        unsigned short tmp_ushort;


        switch (opcode){
        case 0xc3: // jp **
            cpu->pc = imm_16(cpu, ram);
            break;
        case 0x3e: // ld a,*
            byte1 = ram[cpu->pc++];
            cpu->a = byte1;
            break;
        case 0x32: // ld (**), a
            ram[imm_16(cpu, ram)] = cpu->a;
            break;
        case 0x2a: // ld hl, (**)
            cpu->hl = load_16(cpu, ram, imm_16(cpu, ram));
            break;
        case 0xed: // Extended Instructions
            switch(ram[cpu->pc++]){
            case 0x7b: // ld sp, (**)
                cpu->sp = load_16(cpu, ram, imm_16(cpu, ram));
                break;
            case 0xb0: // ldir
                //basically a memcpy
                do ram[cpu->de++] = ram[cpu->hl++];
                while ((unsigned short)--cpu->bc);
                
                break;
            case 0x42: // sbc hl,bc
                // Could be bug here, one of the next 4 lines is probably correct, don't know which
                //cpu->hl = add16(cpu, cpu->hl, ~cpu->bc, cpu->f_c);
                //cpu->hl = add16(cpu, cpu->hl, ~cpu->bc, !cpu->f_c);
                //cpu->hl = add16(cpu, cpu->hl, ~cpu->bc, 2+~cpu->f_c);
                cpu->hl = add16(cpu, cpu->hl, ~(cpu->bc + cpu->f_c), 1);
                cpu->f_n = 1;
                break;
            case 0x57: // some instruction I can skip doing
                break;
            case 0x43: // ld (**),bc
                store_16(cpu, ram, cpu->bc, imm_16(cpu, ram));
                break;
            case 0x53: // ld (**),de
                store_16(cpu, ram, cpu->de, imm_16(cpu, ram));
                break;
            default:
                goto fail;
            }
            break;
        case 0x2b: // dec hl
            cpu->hl--;
            break;
        case 0x56: // ld d, (hl)
            cpu->d = ram[cpu->hl];
            break;
        case 0x5e: // ld e, (hl)
            cpu->e = ram[cpu->hl];
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
            switch(ram[cpu->pc++]){
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
            default:
                goto fail;
            }
            break;
        case 0x1a: // ld a, (de)
            cpu->a = ram[cpu->de];
            break;
        case 0x13: // inc de
            cpu->de++;
            break;
        case 0xfe: // cp *     probably should be something like `cp a,*` or `cp *,a`
            // page 164 in z80 cpu manual
            byte1 = ram[cpu->pc++];
            add8(cpu, cpu->a, ~byte1, 1);
            cpu->f_n = 1;
            break;
        case 0xca: // jp z,**
            tmp_ushort = imm_16(cpu, ram);
            if (cpu->f_z)
                cpu->pc = tmp_ushort;
            break;
        case 0xdd: // IX Instructions
            switch(ram[cpu->pc++]){
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
            default:
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
            byte1 = ram[cpu->pc++];
            cpu->h = byte1;
            break;
        case 0x39: // add hl,sp
            cpu->hl = add16(cpu, cpu->hl, cpu->sp, 0);
            cpu->f_n = 0;
            break;
        case 0x3a: // ld a,(**)
            cpu->a = ram[imm_16(cpu, ram)];
            break;
        case 0xbc: // cp h
            add8(cpu, cpu->a, ~cpu->h, 1);
            cpu->f_n = 1;
            break;
        case 0x30: // jr nc,*
            byte1 = ram[cpu->pc++];
            if(!cpu->f_c)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x46: // ld b,(hl)
            cpu->b = ram[cpu->hl];
            break;
        case 0x24: // inc h
            cpu->h++;
            break;
        case 0x66: // ld h,(hl)
            cpu->h = ram[cpu->hl];
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
            printf("Interrupts off, di instruction not written\n");
            fprintf(fp, "Interrupts off, di instruction not written\n");
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
            ram[cpu->bc] = cpu->a;
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
            cpu->a = ram[cpu->hl];
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
            byte1 = ram[cpu->pc++];
            if(cpu->f_z)
                cpu->pc = (short)(signed char)byte1 + cpu->pc;
            break;
        case 0x09: // add hl,bc
            cpu->hl = add16(cpu, cpu->hl, cpu->bc, 0);
            cpu->f_n = 0;
            break;
        case 0x4e: // ld c,(hl)
            cpu->c = ram[cpu->hl];
            break;
        case 0x06: // ld b,*
            byte1 = ram[cpu->pc++];
            cpu->b = byte1;
            break;
        case 0x18: // jr *
            byte1 = ram[cpu->pc++];
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
            byte1 = ram[cpu->pc++];
            cpu->a &= byte1;
            cpu->f_c = 0;
            cpu->f_n = 0;
            cpu->f_pv = parity(cpu->a);
            cpu->f_h = 1;
            cpu->f_z = !cpu->a;
            cpu->f_s = cpu->a >> 7; // take sign bit and put it in f_s
            break;
        case 0x87: // add a,a
            cpu->a = add8(cpu, cpu->a, cpu->a, 0);
            break;
        case 0xc2: // jp nz,**
            tmp_ushort = imm_16(cpu, ram);
            if (!cpu->f_z)
                cpu->pc = tmp_ushort;
            break;
        case 0x71: // ld (hl),c
            ram[cpu->hl] = cpu->c;
            break;
        case 0x70: // ld (hl),b
            ram[cpu->hl] = cpu->b;
            break;
        case 0x73: // ld (hl),e
            ram[cpu->hl] = cpu->e;
            break;
        case 0x07: // rlca
            cpu->a = cpu->a << 1 | cpu->a >> 7;
            cpu->f_c = cpu->a & 1;
            break;
        case 0xcb:
            switch(ram[cpu->pc++]){
            case 0x3c: // srl h
                cpu->f_c = cpu->h & 1;
                cpu->h >>= 1;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->h;
                cpu->f_s = cpu->h >> 7;
                cpu->f_pv = parity(cpu->h);
                break;
            case 0x1d: // rr l
                cpu->f_c = cpu->l & 1;
                cpu->l = cpu->l >> 1 | cpu->l << 7;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->l;
                cpu->f_s = cpu->l >> 7;
                cpu->f_pv = parity(cpu->l);
                break;
            case 0x10: // rl b
                byte2 = cpu->b;
                cpu->b = cpu->b << 1 | cpu->f_c;
                cpu->f_c = byte2 >> 7;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->b;
                cpu->f_s = cpu->b >> 7;
                cpu->f_pv = parity(cpu->b);
                break;
            case 0x3a: // srl d
                cpu->f_c = cpu->d & 1;
                cpu->d >>= 1;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->d;
                cpu->f_s = cpu->d >> 7;
                cpu->f_pv = parity(cpu->d);
                break;
            case 0x1b: // rr e
                cpu->f_c = cpu->e & 1;
                cpu->e = cpu->e >> 1 | cpu->e << 7;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->e;
                cpu->f_s = cpu->e >> 7;
                cpu->f_pv = parity(cpu->e);
                break;
            case 0x21: // sla c
                cpu->f_c = cpu->c >> 7 & 1;
                cpu->c <<= 1;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->c;
                cpu->f_s = cpu->c >> 7;
                cpu->f_pv = parity(cpu->c);
                break;
            case 0x3b: // srl e
                cpu->f_c = cpu->e & 1;
                cpu->e >>= 1;
                cpu->f_n = 0;
                cpu->f_h = 0;
                cpu->f_z = !cpu->e;
                cpu->f_s = cpu->e >> 7;
                cpu->f_pv = parity(cpu->e);
                break;
            case 0x41: // bit 0,c
                cpu->f_n = 0;
                cpu->f_h = 1;
                cpu->f_z = cpu->c & 1;
                
                break;
            default:
                goto fail;
            }
            break;
        case 0x3d: // dec a
            byte2 = cpu->f_c;
            cpu->a = add8(cpu, cpu->a, (unsigned char)~1, 1);
            cpu->f_c = byte2;
            break;
        case 0x20: // jr nz,*
            byte1 = ram[cpu->pc++];
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
        default:
fail:
            printf("Unknown byte %02hhx at 0x%04hx\n", opcode, oldpc);
            exit(1);
            break;
        }
    }

    fclose(fp);
}


int main(int argc, char const *argv[]) {
    (void)argc;
    unsigned char ram[65536 + 3] = {0}; // 3 for printing opcode bytes easily

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