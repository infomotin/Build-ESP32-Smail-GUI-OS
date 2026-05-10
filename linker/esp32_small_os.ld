/* ESP32 Small OS Linker Script */
MEMORY
{
  /* ESP32 Memory Regions */
  rom (RX) : ORIGIN = 0x40000000, LENGTH = 1M
  iram0 (RWX) : ORIGIN = 0x40020000, LENGTH = 128K
  dram0 (RW) : ORIGIN = 0x3FF80000, LENGTH = 328K
  irom0 (RX) : ORIGIN = 0x400D0000, LENGTH = 1M
  drom0 (R) : ORIGIN = 0x3F400000, LENGTH = 1M
  rtc_fast (RW) : ORIGIN = 0x40070000, LENGTH = 8K
  rtc_slow (RW) : ORIGIN = 0x50000000, LENGTH = 8K
}

/* Stack and heap configuration */
_stack_start = ORIGIN(dram0) + LENGTH(dram0) - 8K;
_stack_end = _stack_start + STACK_SIZE_MAIN;
_heap_start = _stack_end;
_heap_end = ORIGIN(dram0) + LENGTH(dram0);

/* Entry point */
ENTRY(bootloader_main)

/* Sections */
SECTIONS
{
  .text :
  {
    _text_start = .;
    *(.text)
    *(.text.*)
    *(.stub .gnu.linkonce.t.*)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
    *(.glue_7)
    *(.glue_7t)
    *(.gcc_except_table)
    *(.eh_frame)
    _text_end = .;
  } > irom0

  .data :
  {
    _data_start = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    _data_end = .;
  } > dram0 AT > irom0

  .bss :
  {
    _bss_start = .;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    _bss_end = .;
  } > dram0

  /* Heap section */
  .heap (NOLOAD) :
  {
    . = ALIGN(4);
    _heap_start = .;
    . = . + HEAP_SIZE_MAIN;
    _heap_end = .;
  } > dram0

  /* Stack section */
  .stack (NOLOAD) :
  {
    . = ALIGN(4);
    _stack_start = .;
    . = . + STACK_SIZE_MAIN;
    _stack_end = .;
    /* Stack canary for overflow detection */
    LONG(STACK_CANARY)
  } > dram0

  /* DMA buffers */
  .dma_buffers (NOLOAD) :
  {
    . = ALIGN(32);
    _dma_buffers_start = .;
    . = . + (DMA_BUFFER_SIZE * DMA_BUFFER_COUNT);
    _dma_buffers_end = .;
  } > dram0

  /* Interrupt vectors */
  .int_vectors :
  {
    _int_vectors_start = .;
    KEEP(*(.int_vectors))
    _int_vectors_end = .;
  } > iram0

  /* Fast memory sections */
  .fast.text :
  {
    _fast_text_start = .;
    *(.fast.text)
    *(.fast.text.*)
    _fast_text_end = .;
  } > iram0

  .fast.data :
  {
    _fast_data_start = .;
    *(.fast.data)
    *(.fast.data.*)
    _fast_data_end = .;
  } > dram0

  /* Discarded sections */
  /DISCARD/ :
  {
    *(.ARM.exidx)
    *(.ARM.extab)
    *(.comment)
    *(.note)
    *(.note.*)
  }

  /* Memory layout verification */
  .memory_layout :
  {
    LONG(MEMORY_LAYOUT_MAGIC)
    LONG(MEMORY_LAYOUT_VERSION)
    LONG(_text_end - _text_start)
    LONG(_data_end - _data_start)
    LONG(_bss_end - _bss_start)
    LONG(_heap_end - _heap_start)
    LONG(_stack_end - _stack_start)
  } > dram0
}

/* Provide symbols for startup code */
PROVIDE(_text_start = _text_start);
PROVIDE(_text_end = _text_end);
PROVIDE(_data_start = _data_start);
PROVIDE(_data_end = _data_end);
PROVIDE(_bss_start = _bss_start);
PROVIDE(_bss_end = _bss_end);
PROVIDE(_heap_start = _heap_start);
PROVIDE(_heap_end = _heap_end);
PROVIDE(_stack_start = _stack_start);
PROVIDE(_stack_end = _stack_end);

/* Memory region sizes */
_text_size = _text_end - _text_start;
_data_size = _data_end - _data_start;
_bss_size = _bss_end - _bss_start;
_heap_size = _heap_end - _heap_start;
_stack_size = _stack_end - _stack_start;

/* Total memory usage */
_total_memory_usage = _text_size + _data_size + _bss_size + _heap_size + _stack_size;
