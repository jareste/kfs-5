global idt_load
extern idtp

section .text
idt_load:
    lidt [idtp] ; Load the IDT pointer into the CPU
    ret
