/*
 * CRISP-32 Virtual Machine
 * Main implementation
 */

#include "c32_vm.h"
#include "c32_opcodes.h"
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

/* Helper: Check for pending interrupts and dispatch if needed */
static int check_interrupts(c32_vm_t *vm) {
    uint8_t int_num;
    uint8_t byte_idx, bit_idx;
    uint32_t ivt_offset, handler_addr;
    uint32_t i;

    /* Only process interrupts if enabled */
    if (!vm->interrupts.enabled) {
        return 0;
    }

    /* Scan pending interrupt bits (priority order: 0-255) */
    for (int_num = 0; int_num < 255; int_num++) {
        byte_idx = int_num / 8;
        bit_idx = int_num % 8;

        if (vm->interrupts.pending[byte_idx] & (1 << bit_idx)) {
            /* Found pending interrupt - dispatch it */

            /* Clear the pending bit */
            vm->interrupts.pending[byte_idx] &= ~(1 << bit_idx);

            /* Save current PC */
            vm->interrupts.saved_pc = vm->pc;

            /* Switch to kernel mode */
            vm->kernel_mode = 1;

            /* Save all registers to stack (R29 is stack pointer) */
            /* Stack grows downward, allocate 128 bytes (32 regs × 4 bytes) */
            vm->regs[29] -= 128;
            vm->interrupts.saved_regs_addr = vm->regs[29];

            /* Save R0-R31 to guest memory stack */
            if (vm->interrupts.saved_regs_addr + 128 <= vm->memory_size) {
                for (i = 0; i < 32; i++) {
                    c32_write_word(vm->memory + vm->interrupts.saved_regs_addr + (i * 4),
                                   vm->regs[i]);
                }
            }

            /* Disable interrupts */
            vm->interrupts.enabled = 0;

            /* Put interrupt number in R4 (a0) */
            vm->regs[4] = int_num;

            /* Read handler address from IVT */
            ivt_offset = (uint32_t)int_num * 8;
            if (ivt_offset + 4 <= vm->memory_size) {
                handler_addr = c32_read_word(vm->memory + ivt_offset);
                vm->pc = handler_addr;
            } else {
                /* No valid handler - halt */
                vm->running = 0;
                return -1;
            }

            return 1; /* Interrupt dispatched */
        }
    }

    return 0; /* No interrupts pending */
}

/* Helper: Translate virtual address to physical (with paging) */
static uint32_t translate_address(c32_vm_t *vm, uint32_t vaddr, int is_write, int is_exec) {
    uint32_t page_num, page_offset, pte_addr, pte, phys_page, phys_addr;
    uint8_t valid, user, writable, executable;

    /* Kernel mode bypasses paging */
    if (vm->kernel_mode) {
        return vaddr;
    }

    /* Paging disabled - direct mapping */
    if (!vm->paging_enabled) {
        return vaddr;
    }

    /* Extract page number and offset */
    page_num = vaddr >> 12;          /* Bits [31:12] */
    page_offset = vaddr & 0x0FFF;    /* Bits [11:0] */

    /* Check page number bounds */
    if (page_num >= vm->num_pages) {
        /* Page fault: out of bounds */
        c32_raise_interrupt(vm, 8);
        return 0xFFFFFFFF; /* Invalid address marker */
    }

    /* Read PTE from page table */
    pte_addr = vm->page_table_base + (page_num * 4);
    if (pte_addr + 4 > vm->memory_size) {
        /* Page fault: invalid page table */
        c32_raise_interrupt(vm, 8);
        return 0xFFFFFFFF;
    }

    pte = c32_read_word(vm->memory + pte_addr);

    /* Extract PTE fields */
    phys_page = pte & 0xFFFFF000;    /* Bits [31:12] */
    user = (pte >> 3) & 1;           /* Bit 3 */
    executable = (pte >> 2) & 1;     /* Bit 2 */
    writable = (pte >> 1) & 1;       /* Bit 1 */
    valid = pte & 1;                 /* Bit 0 */

    /* Check valid bit */
    if (!valid) {
        /* Page fault: invalid page */
        c32_raise_interrupt(vm, 8);
        return 0xFFFFFFFF;
    }

    /* Check user accessible (user mode requires U bit) */
    if (!user) {
        /* Page fault: not user accessible */
        c32_raise_interrupt(vm, 8);
        return 0xFFFFFFFF;
    }

    /* Check permissions */
    if (is_write && !writable) {
        /* Page fault: write to read-only page */
        c32_raise_interrupt(vm, 8);
        return 0xFFFFFFFF;
    }

    if (is_exec && !executable) {
        /* Page fault: execute on non-executable page */
        c32_raise_interrupt(vm, 8);
        return 0xFFFFFFFF;
    }

    /* Construct physical address */
    phys_addr = phys_page | page_offset;
    return phys_addr;
}

/* VM execution - single instruction step */
int c32_vm_step(c32_vm_t *vm) {
    uint8_t opcode, rs, rt, rd;
    uint32_t imm;
    uint32_t phys_pc;
    const uint8_t *inst;

    /* Check for pending interrupts */
    if (check_interrupts(vm) < 0) {
        return -1; /* Interrupt dispatch failed */
    }

    /* Check PC alignment (8-byte boundary) */
    if ((vm->pc & 0x7) != 0) {
        /* Misaligned PC - raise exception */
        c32_raise_interrupt(vm, 2); /* MEM_FAULT */
        return -1;
    }

    /* Translate PC (virtual to physical) */
    phys_pc = translate_address(vm, vm->pc, 0, 1);
    if (phys_pc == 0xFFFFFFFF) {
        /* Page fault during instruction fetch */
        return -1;
    }

    /* Check PC bounds */
    if (phys_pc + 8 > vm->memory_size) {
        /* PC out of bounds */
        vm->running = 0;
        return -1;
    }

    /* Fetch instruction (8 bytes) */
    inst = vm->memory + phys_pc;
    opcode = inst[0];
    rs = inst[1];
    rt = inst[2];
    rd = inst[3];
    imm = c32_read_word(inst + 4);

    /* Advance PC (will be overridden by branches/jumps) */
    vm->pc += 8;

    /* Decode and execute instruction */
    switch (opcode) {
        /* NOP */
        case OP_NOP:
            break;

        /* Arithmetic - Register-Register */
        case OP_ADD:
            vm->regs[rd] = (int32_t)vm->regs[rs] + (int32_t)vm->regs[rt];
            break;
        case OP_ADDU:
            vm->regs[rd] = vm->regs[rs] + vm->regs[rt];
            break;
        case OP_SUB:
            vm->regs[rd] = (int32_t)vm->regs[rs] - (int32_t)vm->regs[rt];
            break;
        case OP_SUBU:
            vm->regs[rd] = vm->regs[rs] - vm->regs[rt];
            break;

        /* Arithmetic - Register-Immediate */
        case OP_ADDI:
            vm->regs[rt] = (int32_t)vm->regs[rs] + (int32_t)imm;
            break;
        case OP_ADDIU:
            vm->regs[rt] = vm->regs[rs] + imm;
            break;

        /* Logical - Register-Register */
        case OP_AND:
            vm->regs[rd] = vm->regs[rs] & vm->regs[rt];
            break;
        case OP_OR:
            vm->regs[rd] = vm->regs[rs] | vm->regs[rt];
            break;
        case OP_XOR:
            vm->regs[rd] = vm->regs[rs] ^ vm->regs[rt];
            break;
        case OP_NOR:
            vm->regs[rd] = ~(vm->regs[rs] | vm->regs[rt]);
            break;

        /* Logical - Register-Immediate */
        case OP_ANDI:
            vm->regs[rt] = vm->regs[rs] & imm;
            break;
        case OP_ORI:
            vm->regs[rt] = vm->regs[rs] | imm;
            break;
        case OP_XORI:
            vm->regs[rt] = vm->regs[rs] ^ imm;
            break;
        case OP_LUI:
            vm->regs[rt] = imm << 16;
            break;

        /* Shift - Immediate Amount */
        case OP_SLL:
            vm->regs[rd] = vm->regs[rt] << (imm & 0x1F);
            break;
        case OP_SRL:
            vm->regs[rd] = vm->regs[rt] >> (imm & 0x1F);
            break;
        case OP_SRA:
            vm->regs[rd] = (int32_t)vm->regs[rt] >> (imm & 0x1F);
            break;

        /* Shift - Variable Amount */
        case OP_SLLV:
            vm->regs[rd] = vm->regs[rt] << (vm->regs[rs] & 0x1F);
            break;
        case OP_SRLV:
            vm->regs[rd] = vm->regs[rt] >> (vm->regs[rs] & 0x1F);
            break;
        case OP_SRAV:
            vm->regs[rd] = (int32_t)vm->regs[rt] >> (vm->regs[rs] & 0x1F);
            break;

        /* Comparison - Register-Register */
        case OP_SLT:
            vm->regs[rd] = ((int32_t)vm->regs[rs] < (int32_t)vm->regs[rt]) ? 1 : 0;
            break;
        case OP_SLTU:
            vm->regs[rd] = (vm->regs[rs] < vm->regs[rt]) ? 1 : 0;
            break;

        /* Comparison - Register-Immediate */
        case OP_SLTI:
            vm->regs[rt] = ((int32_t)vm->regs[rs] < (int32_t)imm) ? 1 : 0;
            break;
        case OP_SLTIU:
            vm->regs[rt] = (vm->regs[rs] < imm) ? 1 : 0;
            break;

        /* Multiply and Divide */
        case OP_MUL:
            vm->regs[rd] = vm->regs[rs] * vm->regs[rt];
            break;
        case OP_MULH: {
            int32_t a = (int32_t)vm->regs[rs];
            int32_t b = (int32_t)vm->regs[rt];
            /* C89 doesn't have 64-bit, so we can't implement this properly */
            /* For now, just set to 0 */
            vm->regs[rd] = 0;
            (void)a; (void)b;
            break;
        }
        case OP_MULHU:
            /* C89 doesn't have 64-bit, so we can't implement this properly */
            vm->regs[rd] = 0;
            break;
        case OP_DIV:
            if (vm->regs[rt] != 0) {
                vm->regs[rd] = (int32_t)vm->regs[rs] / (int32_t)vm->regs[rt];
            } else {
                vm->regs[rd] = 0; /* Divide by zero returns 0 */
            }
            break;
        case OP_DIVU:
            if (vm->regs[rt] != 0) {
                vm->regs[rd] = vm->regs[rs] / vm->regs[rt];
            } else {
                vm->regs[rd] = 0;
            }
            break;
        case OP_REM:
            if (vm->regs[rt] != 0) {
                vm->regs[rd] = (int32_t)vm->regs[rs] % (int32_t)vm->regs[rt];
            } else {
                vm->regs[rd] = 0;
            }
            break;
        case OP_REMU:
            if (vm->regs[rt] != 0) {
                vm->regs[rd] = vm->regs[rs] % vm->regs[rt];
            } else {
                vm->regs[rd] = 0;
            }
            break;

        /* Load Operations */
        case OP_LW: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 0, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 4 <= vm->memory_size) {
                vm->regs[rt] = c32_read_word(vm->memory + phys_addr);
            }
            break;
        }
        case OP_LH: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 0, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 2 <= vm->memory_size) {
                int16_t val = (int16_t)c32_read_half(vm->memory + phys_addr);
                vm->regs[rt] = (int32_t)val; /* Sign extend */
            }
            break;
        }
        case OP_LHU: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 0, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 2 <= vm->memory_size) {
                vm->regs[rt] = c32_read_half(vm->memory + phys_addr);
            }
            break;
        }
        case OP_LB: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 0, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 1 <= vm->memory_size) {
                int8_t val = (int8_t)c32_read_byte(vm->memory + phys_addr);
                vm->regs[rt] = (int32_t)val; /* Sign extend */
            }
            break;
        }
        case OP_LBU: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 0, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 1 <= vm->memory_size) {
                vm->regs[rt] = c32_read_byte(vm->memory + phys_addr);
            }
            break;
        }

        /* Store Operations */
        case OP_SW: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 1, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 4 <= vm->memory_size) {
                c32_write_word(vm->memory + phys_addr, vm->regs[rt]);
            }
            break;
        }
        case OP_SH: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 1, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 2 <= vm->memory_size) {
                c32_write_half(vm->memory + phys_addr, (uint16_t)vm->regs[rt]);
            }
            break;
        }
        case OP_SB: {
            uint32_t addr = vm->regs[rs] + imm;
            uint32_t phys_addr = translate_address(vm, addr, 1, 0);
            if (phys_addr != 0xFFFFFFFF && phys_addr + 1 <= vm->memory_size) {
                c32_write_byte(vm->memory + phys_addr, (uint8_t)vm->regs[rt]);
            }
            break;
        }

        /* Branch Operations */
        case OP_BEQ:
            if (vm->regs[rs] == vm->regs[rt]) {
                vm->pc = (vm->pc - 8) + imm; /* PC already advanced, so subtract 8 first */
            }
            break;
        case OP_BNE:
            if (vm->regs[rs] != vm->regs[rt]) {
                vm->pc = (vm->pc - 8) + imm;
            }
            break;
        case OP_BLEZ:
            if ((int32_t)vm->regs[rs] <= 0) {
                vm->pc = (vm->pc - 8) + imm;
            }
            break;
        case OP_BGTZ:
            if ((int32_t)vm->regs[rs] > 0) {
                vm->pc = (vm->pc - 8) + imm;
            }
            break;
        case OP_BLTZ:
            if ((int32_t)vm->regs[rs] < 0) {
                vm->pc = (vm->pc - 8) + imm;
            }
            break;
        case OP_BGEZ:
            if ((int32_t)vm->regs[rs] >= 0) {
                vm->pc = (vm->pc - 8) + imm;
            }
            break;

        /* Jump Operations */
        case OP_J:
            vm->pc = imm;
            break;
        case OP_JAL:
            vm->regs[31] = vm->pc; /* PC already advanced by 8 */
            vm->pc = imm;
            break;
        case OP_JR:
            vm->pc = vm->regs[rs];
            break;
        case OP_JALR:
            vm->regs[rd] = vm->pc; /* PC already advanced by 8 */
            vm->pc = vm->regs[rs];
            break;

        /* System Operations */
        case OP_SYSCALL:
            c32_raise_interrupt(vm, 4); /* SYSCALL interrupt */
            vm->running = 0; /* Stop execution */
            break;
        case OP_BREAK:
            c32_raise_interrupt(vm, 5); /* BREAK interrupt */
            vm->running = 0;
            break;

        /* Interrupt Control Operations */
        case OP_EI:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7); /* PRIVILEGE_VIOLATION */
            } else {
                vm->interrupts.enabled = 1;
            }
            break;
        case OP_DI:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7);
            } else {
                vm->interrupts.enabled = 0;
            }
            break;
        case OP_IRET:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7);
            } else {
                uint32_t i;
                /* Restore PC */
                vm->pc = vm->interrupts.saved_pc;
                /* Restore all registers from stack */
                if (vm->interrupts.saved_regs_addr + 128 <= vm->memory_size) {
                    for (i = 0; i < 32; i++) {
                        vm->regs[i] = c32_read_word(vm->memory +
                                                    vm->interrupts.saved_regs_addr + (i * 4));
                    }
                }
                /* Enable interrupts */
                vm->interrupts.enabled = 1;
                /* Note: In a real implementation, we might restore privilege level here */
            }
            break;
        case OP_RAISE:
            c32_raise_interrupt(vm, (uint8_t)imm);
            break;
        case OP_GETPC:
            vm->regs[rd] = vm->interrupts.saved_pc;
            break;

        /* Privilege and MMU Control */
        case OP_ENABLE_PAGING:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7);
            } else {
                vm->paging_enabled = 1;
            }
            break;
        case OP_DISABLE_PAGING:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7);
            } else {
                vm->paging_enabled = 0;
            }
            break;
        case OP_SET_PTBR:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7);
            } else {
                vm->page_table_base = vm->regs[rd];
                vm->num_pages = vm->regs[rt];
            }
            break;
        case OP_ENTER_USER:
            if (!vm->kernel_mode) {
                c32_raise_interrupt(vm, 7);
            } else {
                vm->kernel_mode = 0;
            }
            break;
        case OP_GETMODE:
            vm->regs[rd] = vm->kernel_mode;
            break;

        /* Unknown opcode */
        default:
            c32_raise_interrupt(vm, 1); /* ILLEGAL_OP */
            vm->running = 0;
            break;
    }

    /* Enforce R0 = 0 */
    vm->regs[0] = 0;

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
