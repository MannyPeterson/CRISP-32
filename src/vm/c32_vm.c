/*
 * CRISP-32 Virtual Machine
 * Main implementation
 */

#include "c32_vm.h"
#include "c32_string.h"

/* Endianness detection and conversion */
#ifdef C32_HOST_BIG_ENDIAN
#define NEED_ENDIAN_SWAP 1

/* Byte swap helpers - only needed on big-endian hosts */
static uint32_t swap32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

static uint16_t swap16(uint16_t val) {
    return ((val & 0xFF00) >> 8) |
           ((val & 0x00FF) << 8);
}

#else
#define NEED_ENDIAN_SWAP 0
#endif

/* Memory access functions with endianness handling */
uint32_t c32_read_word(const uint8_t *addr) {
    uint32_t val;
    c32_memcpy(&val, addr, 4);
#if NEED_ENDIAN_SWAP
    return swap32(val);
#else
    return val;
#endif
}

uint16_t c32_read_half(const uint8_t *addr) {
    uint16_t val;
    c32_memcpy(&val, addr, 2);
#if NEED_ENDIAN_SWAP
    return swap16(val);
#else
    return val;
#endif
}

uint8_t c32_read_byte(const uint8_t *addr) {
    return *addr;
}

void c32_write_word(uint8_t *addr, uint32_t value) {
#if NEED_ENDIAN_SWAP
    uint32_t val = swap32(value);
    c32_memcpy(addr, &val, 4);
#else
    c32_memcpy(addr, &value, 4);
#endif
}

void c32_write_half(uint8_t *addr, uint16_t value) {
#if NEED_ENDIAN_SWAP
    uint16_t val = swap16(value);
    c32_memcpy(addr, &val, 2);
#else
    c32_memcpy(addr, &value, 2);
#endif
}

void c32_write_byte(uint8_t *addr, uint8_t value) {
    *addr = value;
}

/* VM initialization */
void c32_vm_init(c32_vm_t *vm, uint8_t *memory, uint32_t memory_size) {
    size_t i;

    /* Clear registers */
    for (i = 0; i < 32; i++) {
        vm->regs[i] = 0;
    }

    vm->pc = 0;
    vm->memory = memory;
    vm->memory_size = memory_size;
    vm->running = 0;

    /* Start in kernel mode with paging disabled */
    vm->kernel_mode = 1;
    vm->paging_enabled = 0;
    vm->page_table_base = 0;
    vm->num_pages = 0;

    /* Clear interrupt state */
    vm->interrupts.enabled = 0;
    for (i = 0; i < 32; i++) {
        vm->interrupts.pending[i] = 0;
    }
    vm->interrupts.saved_pc = 0;
    vm->interrupts.saved_regs_addr = 0;
}

void c32_vm_reset(c32_vm_t *vm) {
    size_t i;

    for (i = 0; i < 32; i++) {
        vm->regs[i] = 0;
    }

    vm->pc = 0;
    vm->running = 0;
    vm->kernel_mode = 1;
    vm->paging_enabled = 0;
}

/* Interrupt management */
void c32_raise_interrupt(c32_vm_t *vm, uint8_t int_num) {
    uint8_t byte_idx = int_num / 8;
    uint8_t bit_idx = int_num % 8;

    if (byte_idx < 32) {
        vm->interrupts.pending[byte_idx] |= (1 << bit_idx);
    }
}

void c32_set_interrupt_handler(c32_vm_t *vm, uint8_t int_num, uint32_t handler_addr) {
    uint32_t ivt_offset;

    /* Calculate byte offset in guest memory: int_num * 8 */
    /* Note: int_num is uint8_t, so no need to check >= 256 */
    ivt_offset = (uint32_t)int_num * 8;

    /* Write handler address to guest memory (little-endian) */
    if (ivt_offset + 4 <= vm->memory_size) {
        c32_write_word(vm->memory + ivt_offset, handler_addr);
    }
}

/* VM execution - stub for now */
int c32_vm_step(c32_vm_t *vm) {
    /* TODO: Implement instruction fetch, decode, and execute */
    (void)vm;
    return 0;
}

void c32_vm_run(c32_vm_t *vm) {
    vm->running = 1;

    while (vm->running) {
        if (c32_vm_step(vm) != 0) {
            break;
        }
    }
}
