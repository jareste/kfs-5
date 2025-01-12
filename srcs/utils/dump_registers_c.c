#include "../display/display.h"

void dump_registers_c(registers_t* regs)
{
    puts("EAX:    0x");
    put_hex(regs->eax);
    puts("\n");

    puts("EBX:    0x");
    put_hex(regs->ebx);
    puts("\n");

    puts("ECX:    0x");
    put_hex(regs->ecx);
    puts("\n");

    puts("EDX:    0x");
    put_hex(regs->edx);
    puts("\n");

    puts("ESI:    0x");
    put_hex(regs->esi);
    puts("\n");

    puts("EDI:    0x");
    put_hex(regs->edi);
    puts("\n");

    puts("EBP:    0x");
    put_hex(regs->ebp);
    puts("\n");

    puts("ESP:    0x");
    put_hex(regs->esp);
    puts("\n");

    puts("EIP:    0x");
    put_hex(regs->eip);
    puts("\n");

    puts("EFLAGS: 0x");
    put_hex(regs->eflags);
    puts("\n");

    puts("CS:     0x");
    put_hex(regs->cs);
    puts("\n");

    puts("DS:     0x");
    put_hex(regs->ds);
    puts("\n");

    puts("ES:     0x");
    put_hex(regs->es);
    puts("\n");

    puts("FS:     0x");
    put_hex(regs->fs);
    puts("\n");

    puts("GS:     0x");
    put_hex(regs->gs);
    puts("\n");

    puts("SS:     0x");
    put_hex(regs->ss);
    puts("\n");   
}
