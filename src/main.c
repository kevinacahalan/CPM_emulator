#include <stdio.h>
#include <stdlib.h>

unsigned char ram[65536] = {0};

struct cpu{
    unsigned short pc; // instruction pointer
    union{
        unsigned short af;
        struct{
            unsigned char f;
            unsigned char a;
        };
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

    
    for(;;){
        unsigned char opcode = ram[cpu->pc];
        unsigned char oldpc = cpu->pc++; // oldpc gets cpu->pc before it is instrumented
        unsigned char byte1;
        unsigned char byte2;


        switch (opcode)
        {
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
        
        default:
            printf("Unknown byte %02hhx at 0x%04hx\n", opcode, oldpc);
            exit(1);
            break;
        }
    }

    return 0;
}
