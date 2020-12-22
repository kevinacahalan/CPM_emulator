#include <stdio.h>
#include <stdlib.h>

unsigned char ram[65536 + 2] = {0}; // 2 for printing opcode bytes easily

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
            unsigned short f_c : 1;
            unsigned short f_n : 1;
            unsigned short f_pv : 1;
            unsigned short f_bit3 : 1;
            unsigned short f_h : 1;
            unsigned short f_bit5 : 1;
            unsigned short f_z : 1;
            unsigned short f_s : 1;
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

};


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
        unsigned short oldpc = cpu->pc++; // oldpc gets cpu->pc before it is instrumented
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
        //case 0xfe: // cp *     probably should be something like `cp a,*` or `cp *,a`
            
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