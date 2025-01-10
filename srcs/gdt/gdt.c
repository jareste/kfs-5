#include "gdt.h"
#include "../display/display.h"

gdt_entry_t gdt[GDT_ENTRIES];
gdt_ptr_t* gdt_ptr = (gdt_ptr_t*)GDT_ADDRESS;

/* Assembly function to load the GDT */
extern void gdt_flush();

void register_gdt(void)
{
    gdt_ptr->base = (uint32_t) &gdt;
    gdt_ptr->limit = (GDT_ENTRIES * sizeof(gdt)) - 1;
    __asm__ __volatile__("lgdtl (%0)" : : "r" (gdt_ptr));
    gdt_flush();
}

void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= (granularity & 0xF0);
    gdt[index].access = access;
}


void gdt_init()
{
    /* NULL */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* Kernel Code segment */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Kernel Data segment */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* Kernel Stack */
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0x96, 0xCF);

    /* User mode code segment */
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* User mode data segment */
    gdt_set_entry(5, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* User mode stack */
    gdt_set_entry(6, 0, 0xFFFFFFFF, 0xF6, 0xCF);

    register_gdt();
}
