#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


unsigned char ram[65536 + 3] = {0}; // 3 for printing opcode bytes easily

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
    unsigned short bc_prime;
    unsigned short de_prime;
    unsigned short hl_prime;
};

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
    int carry_out = usum != (uint8_t)usum;
    // int overflow = ssum != (int8_t)ssum;
    
    uint8_t result = usum;
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


int main(int argc, char const *argv[]) {
    (void)argc;
    FILE *fp = fopen(argv[1], "rb");
    if (!fp){
        puts("No input file");
        return 1;
    }

    printf("got %zd bytes\n", fread(ram + 256, 1, 65536 - 256, fp));
    fclose(fp);
    
    struct cpu *cpu = calloc(sizeof *cpu, 1);
    cpu->pc = 256; //where CPM starts program

    unsigned long long ran = 0;
    for(;;){
        printf("Bytes %02hhx %02hhx %02hhx %02hhx at 0x%04hx after %llu run\n",ram[cpu->pc],ram[cpu->pc+1],ram[cpu->pc+2],ram[cpu->pc+3],cpu->pc,ran++);
        unsigned char opcode = ram[cpu->pc];
        unsigned short oldpc = cpu->pc++; // increment instruction pointer
        unsigned char byte1;
        unsigned char byte2;
        unsigned short tmp_short;


        switch (opcode){
        case 0xc3: // jp **
            byte1 = ram[cpu->pc++];
            byte2 = ram[cpu->pc++];
            cpu->pc = byte1 | (byte2 << 8);
            break;
        case 0x3e: // ld a,*
            byte1 = ram[cpu->pc++];
            cpu->a = byte1;
            break;
        case 0x32: // ld (**), a
            byte1 = ram[cpu->pc++];
            byte2 = ram[cpu->pc++];
            ram[byte1 | (byte2 << 8)] = cpu->a;
            break;
        case 0x2a: // ld hl, (**)
            byte1 = ram[cpu->pc++];
            byte2 = ram[cpu->pc++];
            cpu->hl = ram[byte1 | (byte2 << 8)];
            break;
        case 0xed: // Extended Instructions
            switch(ram[cpu->pc++]){
            case 0x7b: // ld sp, (**)
                byte1 = ram[cpu->pc++];
                byte2 = ram[cpu->pc++];
                cpu->sp = ram[byte1 | (byte2 << 8)];
                cpu->sp |= ram[(byte1 | (byte2 << 8)) + 1] << 8;
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
            tmp_short = cpu->de;
            cpu->de = cpu->hl;
            cpu->hl = tmp_short;
            break;
        case 0x23: // inc hl
            cpu->hl++;
            break;
        case 0x19: // add hl, de
            cpu->f_h = ((cpu->hl & 0x0fff) + (cpu->de & 0x0fff)) >> 12;
            cpu->hl += cpu->de;
            break;
        case 0xd5: // push de
            cpu->sp--;
            ram[cpu->sp] = cpu->d;
            cpu->sp--;
            ram[cpu->sp] = cpu->e;
            break;
        case 0x01: // ld bc, **
            byte1 = ram[cpu->pc++];
            byte2 = ram[cpu->pc++];
            cpu->bc = byte1 | (byte2 << 8);
            break;
        case 0xfd: // IY Instructions
            switch(ram[cpu->pc++]){
            case 0x21: // ld iy, **
                byte1 = ram[cpu->pc++];
                byte2 = ram[cpu->pc++];
                cpu->iy = byte1 | (byte2 << 8);
                break;
            case 0xe9: // jp (iy) ...the syntex of this instruction is off
                cpu->pc = cpu->iy;
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
            byte1 = ram[cpu->pc++];
            byte2 = ram[cpu->pc++];
            if (cpu->f_z)
                cpu->pc = byte1 | (byte2 << 8);
            break;
        case 0xdd: // IX Instructions
            switch(ram[cpu->pc++]){
            case 0xe5: // push ix
            // page 117 om z80 cpu manual
                cpu->sp--;
                ram[cpu->sp] = cpu->ix>>8; // push high bits
                cpu->sp--;
                ram[cpu->sp] = cpu->ix; // push low bits
                break;
            case 0x21: // ld ix,**
                byte1 = ram[cpu->pc++];
                byte2 = ram[cpu->pc++];
                cpu->ix = byte1 | (byte2 << 8);
                break;
            case 0x39: // add ix,sp
                cpu->ix = add16(cpu, cpu->ix, cpu->sp, 0);
                cpu->f_n = 1;
                break;
            default:
                goto fail;
            }
            break;
        case 0xc5: // push bc
            cpu->sp--;
            ram[cpu->sp] = cpu->b;
            cpu->sp--;
            ram[cpu->sp] = cpu->c;
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
            cpu->f_n = 1;
            break;
        case 0x3a: // ld a,(**)
            byte1 = ram[cpu->pc++];
            byte2 = ram[cpu->pc++];
            cpu->a = ram[byte1 | (byte2 << 8)];
            break;
        case 0xbc: // cp h
            add8(cpu, cpu->a, ~cpu->h, 1);
            cpu->f_n = 1;
            break;
        case 0x30: // jr nc,*
            byte1 = ram[cpu->pc++];
            if(!cpu->f_c)
                cpu->pc = (short)(signed char)byte1 + oldpc;
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
            tmp_short = cpu->bc;
            cpu->bc = cpu->bc_prime;
            cpu->bc_prime = tmp_short;

            tmp_short = cpu->de;
            cpu->de = cpu->de_prime;
            cpu->de_prime = tmp_short;

            tmp_short = cpu->hl;
            cpu->hl = cpu->hl_prime;
            cpu->hl_prime = tmp_short;

            break;
        default:
fail:
            printf("Unknown byte %02hhx at 0x%04hx\n", opcode, oldpc);
            exit(1);
            break;
        }
    }

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