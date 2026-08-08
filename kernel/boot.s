; --- Nexus OS Bootloader ---
; Multiboot compliant x86 bootstrapper

MBALIGN  equ  1 << 0              ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1              ; provide memory map
FLAGS    equ  MBALIGN | MEMINFO  ; multiboot 'flag' field
MAGIC    equ  0x1BADB002          ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)     ; checksum of above, to prove we are multiboot

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:

section .text
global _start:function (_start.end - _start)
_start:
    ; Setup the stack
    mov esp, stack_top

    ; Push multiboot info pointers for the C kernel
    push ebx
    push eax

    ; Call the kernel main
    extern kernel_main
    call kernel_main

    ; If kernel returns, halt the CPU
    cli
.hang:
    hlt
    jmp .hang
.end:

global gdt_flush
extern gp
gdt_flush:
    lgdt [gp]        ; Load the GDT with the pointer to the GDT structure
    mov ax, 0x10     ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax       ; Load all data segment selectors
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush  ; 0x08 is the offset to our code segment: Far jump!
.flush:
    ret

global idt_load
extern idtp
idt_load:
    lidt [idtp]
    ret

; ISR/IRQ Common Stubs
global isr_common_stub
extern isr_handler
isr_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call isr_handler
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

global irq_common_stub
extern irq_handler
irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call irq_handler
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; Individual Stubs
global isr0
isr0:
    push byte 0
    push byte 0
    jmp isr_common_stub

global irq0
irq0:
    push byte 0
    push byte 32
    jmp irq_common_stub

global irq1
irq1:
    push byte 0
    push byte 33
    jmp irq_common_stub
