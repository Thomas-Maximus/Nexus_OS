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
global stack_bottom
stack_bottom:
    resb 65536 ; 64 KiB
global stack_top
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
    push esp         ; Pass pointer to struct regs (esp) to isr_handler
    call isr_handler
    add esp, 4       ; Pop the pushed esp argument
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
    push esp         ; Pass pointer to struct regs (esp) to irq_handler
    call irq_handler
    add esp, 4       ; Pop the pushed esp argument
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; Individual ISR Stubs (CPU Exceptions 0-19)
; Some exceptions push an error code, some don't — we push a dummy 0 for consistency
global isr0
isr0:  push byte 0 ; #DE Divide Error
       push byte 0 ; int_no
       jmp isr_common_stub

global isr1
isr1:  push byte 0 ; #DB Debug
       push byte 1
       jmp isr_common_stub

global isr2
isr2:  push byte 0 ; NMI
       push byte 2
       jmp isr_common_stub

global isr3
isr3:  push byte 0 ; #BP Breakpoint
       push byte 3
       jmp isr_common_stub

global isr4
isr4:  push byte 0 ; #OF Overflow
       push byte 4
       jmp isr_common_stub

global isr5
isr5:  push byte 0 ; #BR Bound Range
       push byte 5
       jmp isr_common_stub

global isr6
isr6:  push byte 0 ; #UD Invalid Opcode
       push byte 6
       jmp isr_common_stub

global isr7
isr7:  push byte 0 ; #NM Device Not Available
       push byte 7
       jmp isr_common_stub

global isr8
isr8:            ; #DF Double Fault — CPU pushes error code
       push byte 8
       jmp isr_common_stub

global isr9
isr9:  push byte 0 ; Coprocessor Segment Overrun (legacy)
       push byte 9
       jmp isr_common_stub

global isr10
isr10:           ; #TS Invalid TSS — CPU pushes error code
       push byte 10
       jmp isr_common_stub

global isr11
isr11:           ; #NP Segment Not Present — CPU pushes error code
       push byte 11
       jmp isr_common_stub

global isr12
isr12:           ; #SS Stack Fault — CPU pushes error code
       push byte 12
       jmp isr_common_stub

global isr13
isr13:           ; #GP General Protection — CPU pushes error code
       push byte 13
       jmp isr_common_stub

global isr14
isr14:           ; #PF Page Fault — CPU pushes error code
       push byte 14
       jmp isr_common_stub

global isr15
isr15: push byte 0
       push byte 15
       jmp isr_common_stub

global isr16
isr16: push byte 0 ; #MF x87 FP Exception
       push byte 16
       jmp isr_common_stub

global isr17
isr17:           ; #AC Alignment Check — CPU pushes error code
       push byte 17
       jmp isr_common_stub

global isr18
isr18: push byte 0 ; #MC Machine Check
       push byte 18
       jmp isr_common_stub

global isr19
isr19: push byte 0 ; #XM SIMD FP Exception
       push byte 19
       jmp isr_common_stub

; IRQ Stubs (Hardware Interrupts 0-15, remapped to INT 32-47)
global irq0
irq0:  push byte 0
       push byte 32
       jmp irq_common_stub

global irq1
irq1:  push byte 0
       push byte 33
       jmp irq_common_stub

global irq2
irq2:  push byte 0
       push byte 34
       jmp irq_common_stub

global irq3
irq3:  push byte 0
       push byte 35
       jmp irq_common_stub

global irq4
irq4:  push byte 0
       push byte 36
       jmp irq_common_stub

global irq5
irq5:  push byte 0
       push byte 37
       jmp irq_common_stub

global irq6
irq6:  push byte 0
       push byte 38
       jmp irq_common_stub

global irq7
irq7:  push byte 0
       push byte 39
       jmp irq_common_stub

global irq8
irq8:  push byte 0
       push byte 40
       jmp irq_common_stub

global irq9
irq9:  push byte 0
       push byte 41
       jmp irq_common_stub

global irq10
irq10: push byte 0
       push byte 42
       jmp irq_common_stub

global irq11
irq11: push byte 0
       push byte 43
       jmp irq_common_stub

global irq12
irq12: push byte 0
       push byte 44
       jmp irq_common_stub

global irq13
irq13: push byte 0
       push byte 45
       jmp irq_common_stub

global irq14
irq14: push byte 0
       push byte 46
       jmp irq_common_stub

global irq15
irq15: push byte 0
       push byte 47
       jmp irq_common_stub

section .note.GNU-stack noalloc noexec nowrite progbits

