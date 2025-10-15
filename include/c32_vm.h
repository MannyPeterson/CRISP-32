/*
 * CRISP-32 Virtual Machine
 * Freestanding C89 Implementation
 */

#ifndef C32_VM_H
#define C32_VM_H

#include "c32_types.h"

/* VM State Structure */
typedef struct {
    uint32_t regs[32];     /* R0-R31 (current values) */
    uint32_t pc;           /* Program counter (current) */
    uint8_t *memory;       /* Pointer to guest memory */
    uint32_t memory_size;  /* Memory size in bytes */
    int running;           /* Execution state */

    /* Privilege state */
    uint8_t kernel_mode;   /* 1 = kernel, 0 = user */

    /* Paging state (MMU control) */
    uint8_t paging_enabled;   /* 1 = virtual addressing on */
    uint32_t page_table_base; /* Physical address of page table in guest memory */
    uint32_t num_pages;       /* Number of virtual pages */

    /* Interrupt state */
    struct {
        uint8_t enabled;           /* Global interrupt enable */
        uint8_t pending[32];       /* Pending interrupts (256 bits) */
        uint32_t saved_pc;         /* Saved PC when interrupt fires */
        uint32_t saved_regs_addr;  /* Address in guest memory where R0-R31 saved */
    } interrupts;
} c32_vm_t;

/* VM API Functions */
void c32_vm_init(c32_vm_t *vm, uint8_t *memory, uint32_t memory_size);
void c32_vm_reset(c32_vm_t *vm);
int c32_vm_step(c32_vm_t *vm);
void c32_vm_run(c32_vm_t *vm);

/* Interrupt management */
void c32_raise_interrupt(c32_vm_t *vm, uint8_t int_num);
void c32_set_interrupt_handler(c32_vm_t *vm, uint8_t int_num, uint32_t handler_addr);

/* Memory access helpers */
uint32_t c32_read_word(const uint8_t *addr);
uint16_t c32_read_half(const uint8_t *addr);
uint8_t c32_read_byte(const uint8_t *addr);
void c32_write_word(uint8_t *addr, uint32_t value);
void c32_write_half(uint8_t *addr, uint16_t value);
void c32_write_byte(uint8_t *addr, uint8_t value);

#endif /* C32_VM_H */
