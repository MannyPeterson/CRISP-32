/**
 * @file c32_vm.h
 * @brief CRISP-32 Virtual Machine Core API
 * @author Manny Peterson <manny@manny.ca>
 * @date 2025
 * @copyright Copyright (C) 2025 Manny Peterson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef C32_VM_H
#define C32_VM_H

#include "c32_types.h"

/**
 * @defgroup vm_core Virtual Machine Core
 * @brief Complete CRISP-32 virtual machine implementation
 *
 * This module provides a complete freestanding implementation of the
 * CRISP-32 instruction set architecture, including privilege levels,
 * paging/MMU, and interrupt handling.
 * @{
 */

/**
 * @brief VM State Structure
 *
 * Complete virtual machine state including registers, memory,
 * privilege level, paging, and interrupt subsystem.
 */
typedef struct {
    /** General purpose registers R0-R31 (R0 is hardwired to zero) */
    uint32_t regs[32];

    /** Program counter (must be 8-byte aligned) */
    uint32_t pc;

    /** Pointer to guest physical memory */
    uint8_t *memory;

    /** Total memory size in bytes */
    uint32_t memory_size;

    /** Execution state: 1 = running, 0 = halted */
    int running;

    /** Privilege level: 1 = kernel mode, 0 = user mode */
    uint8_t kernel_mode;

    /** Paging enabled flag: 1 = virtual addressing on, 0 = physical only */
    uint8_t paging_enabled;

    /** Physical address of page table in guest memory */
    uint32_t page_table_base;

    /** Number of virtual pages managed by MMU */
    uint32_t num_pages;

    /** Interrupt subsystem state */
    struct {
        /** Global interrupt enable flag */
        uint8_t enabled;

        /** Pending interrupt bitmap (256 bits = 32 bytes) */
        uint8_t pending[32];

        /** Saved PC when interrupt fires */
        uint32_t saved_pc;

        /** Address in guest memory where R0-R31 are saved */
        uint32_t saved_regs_addr;
    } interrupts;
} c32_vm_t;

/**
 * @defgroup vm_lifecycle VM Lifecycle
 * @brief VM initialization, reset, and execution
 * @{
 */

/**
 * @brief Initialize virtual machine
 *
 * Sets up the VM with guest memory and resets all state.
 * Must be called before using the VM.
 *
 * @param vm Pointer to VM structure
 * @param memory Pointer to guest physical memory buffer
 * @param memory_size Size of guest memory in bytes
 */
void c32_vm_init(c32_vm_t *vm, uint8_t *memory, uint32_t memory_size);

/**
 * @brief Reset virtual machine to initial state
 *
 * Clears all registers, sets PC to 0, enters kernel mode,
 * disables paging, and clears interrupt state.
 *
 * @param vm Pointer to VM structure
 */
void c32_vm_reset(c32_vm_t *vm);

/**
 * @brief Execute single instruction
 *
 * Fetches, decodes, and executes one instruction at the current PC.
 * Advances PC by 8 bytes. Checks for interrupts before execution.
 *
 * @param vm Pointer to VM structure
 * @return 1 if instruction executed successfully, 0 if VM halted
 */
int c32_vm_step(c32_vm_t *vm);

/**
 * @brief Run VM until halted
 *
 * Executes instructions in a loop until the VM halts
 * (running flag becomes 0).
 *
 * @param vm Pointer to VM structure
 */
void c32_vm_run(c32_vm_t *vm);

/** @} */ /* end of vm_lifecycle */

/**
 * @defgroup vm_interrupts Interrupt Management
 * @brief Interrupt control and handling
 * @{
 */

/**
 * @brief Raise software interrupt
 *
 * Sets the pending bit for the specified interrupt number.
 * The interrupt will be dispatched before the next instruction
 * if interrupts are enabled.
 *
 * @param vm Pointer to VM structure
 * @param int_num Interrupt number (0-255)
 */
void c32_raise_interrupt(c32_vm_t *vm, uint8_t int_num);

/**
 * @brief Set interrupt handler address
 *
 * Writes the handler address to the Interrupt Vector Table (IVT)
 * in guest memory at address (int_num * 4). The IVT is located
 * at physical address 0x00000000.
 *
 * @param vm Pointer to VM structure
 * @param int_num Interrupt number (0-255)
 * @param handler_addr Physical address of interrupt handler
 */
void c32_set_interrupt_handler(c32_vm_t *vm, uint8_t int_num, uint32_t handler_addr);

/** @} */ /* end of vm_interrupts */

/**
 * @defgroup vm_memory Memory Access Helpers
 * @brief Little-endian memory read/write functions
 *
 * These functions handle endianness conversion automatically.
 * All multi-byte values in CRISP-32 are stored in little-endian format.
 * @{
 */

/**
 * @brief Read 32-bit word from memory
 *
 * Reads 4 bytes in little-endian order and converts to host byte order.
 *
 * @param addr Pointer to memory location
 * @return 32-bit value in host byte order
 */
uint32_t c32_read_word(const uint8_t *addr);

/**
 * @brief Read 16-bit halfword from memory
 *
 * Reads 2 bytes in little-endian order and converts to host byte order.
 *
 * @param addr Pointer to memory location
 * @return 16-bit value in host byte order
 */
uint16_t c32_read_half(const uint8_t *addr);

/**
 * @brief Read 8-bit byte from memory
 *
 * Reads a single byte.
 *
 * @param addr Pointer to memory location
 * @return 8-bit value
 */
uint8_t c32_read_byte(const uint8_t *addr);

/**
 * @brief Write 32-bit word to memory
 *
 * Converts value to little-endian order and writes 4 bytes.
 *
 * @param addr Pointer to memory location
 * @param value 32-bit value in host byte order
 */
void c32_write_word(uint8_t *addr, uint32_t value);

/**
 * @brief Write 16-bit halfword to memory
 *
 * Converts value to little-endian order and writes 2 bytes.
 *
 * @param addr Pointer to memory location
 * @param value 16-bit value in host byte order
 */
void c32_write_half(uint8_t *addr, uint16_t value);

/**
 * @brief Write 8-bit byte to memory
 *
 * Writes a single byte.
 *
 * @param addr Pointer to memory location
 * @param value 8-bit value
 */
void c32_write_byte(uint8_t *addr, uint8_t value);

/** @} */ /* end of vm_memory */

/** @} */ /* end of vm_core */

#endif /* C32_VM_H */
