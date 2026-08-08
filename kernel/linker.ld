/* --- Nexus OS Linker Script --- */
ENTRY(_start)

SECTIONS
{
	/* Begin putting sections at 1MiB */
	. = 1M;

	/* Multiboot header must be at the very beginning */
	.text :
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data */
	.rodata :
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data :
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss :
	{
		*(COMMON)
		*(.bss)
	}
}
