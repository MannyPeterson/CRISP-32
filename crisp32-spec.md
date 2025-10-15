# CRISP-32 ISA Specification
**Clean RISC Instruction Set Platform - 32-bit Architecture**

Version 1.0

---

## 6. Privilege Levels and Memory Protection

CRISP-32 implements a simple two-level privilege system integrated with optional paging for memory protection.

### 6.1 Privilege Levels

**Two privilege levels:**
- **Kernel mode** (`kernel_mode = 1`): Full access to all instructions and memory
- **User mode** (`kernel_mode = 0`): Restricted access

**Privilege state:**
- Stored in VM structure (not directly accessible to guest programs)
- Can be read using `GETMODE` instruction
- Changed automatically by hardware on mode transitions

### 6.2 Privileged Instructions

The following instructions can only execute in kernel mode. Attempting to execute them in user mode raises interrupt 7 (PRIVILEGE_VIOLATION):

- `EI` - Enable interrupts
- `DI` - Disable interrupts
- `IRET` - Return from interrupt
- `ENABLE_PAGING` - Enable virtual addressing
- `DISABLE_PAGING` - Disable virtual addressing
- `SET_PTBR` - Set page table base and size
- `ENTER_USER` - Switch to user mode

### 6.3 Mode Transitions

**User → Kernel** (automatic):
- Any interrupt (including SYSCALL, BREAK)
- Sets `kernel_mode = 1`
- Saves all context to guest memory stack

**Kernel → User** (explicit):
- `ENTER_USER` instruction
- Sets `kernel_mode = 0`
- `IRET` instruction (restores previous mode)

**Example mode transition:**
```assembly
; In kernel mode
setup_user_task:
    ; Setup user environment
    ; ... initialize registers, stack, etc ...
    
    ; Drop to user mode
    ENTER_USER
    
    ; Now in user mode - restricted access
    ; User code runs here...
    
    ; User makes system call
    SYSCALL
    
    ; Automatically back in kernel mode
    ; Handle syscall...
    
    ; Return to user mode
    IRET
```

### 6.4 Virtual Memory and Paging

CRISP-32 supports optional virtual memory with a simple flat page table.

**Page size**: 4KB (4096 bytes)

**Virtual address format:**
```
[31:12] Page number (20 bits)
[11:0]  Page offset (12 bits)
```

**Page table entry (PTE) format** (32 bits):
```
[31:12] Physical page number (20 bits)
[11:4]  Reserved (8 bits)
[3]     User accessible (U bit)
[2]     Executable (X bit)
[1]     Writable (W bit)
[0]     Valid (V bit)
```

### 6.5 Address Translation Rules

**Kernel mode:**
- Virtual addresses = physical addresses (no translation)
- Direct access to all physical memory
- Bypasses paging entirely

**User mode with paging disabled:**
- Virtual addresses = physical addresses (no translation)
- No memory protection
- User can access all physical memory

**User mode with paging enabled:**
- Virtual addresses translated through page table
- Page permissions enforced
- Only pages with U bit set are accessible

### 6.6 Page Table

**Location:**
- Stored in guest memory at physical address `page_table_base`
- Set by kernel using `SET_PTBR` instruction

**Structure:**
- Flat array of 32-bit page table entries
- Entry for page N at: `page_table_base + (N × 4)`
- Size: `num_pages × 4` bytes

**Example page table setup:**
```c
/* Kernel sets up 4MB virtual address space (1024 pages) */
/* Page table at physical address 0x9000 */

/* Set page table base */
SET_PTBR R5, R6    /* R5 = 0x9000, R6 = 1024 */

/* Map virtual page 0 to physical 0xA000 with U|W|X|V permissions */
LUI  R7, 0
ORI  R7, R7, 0x9000    /* R7 = page table base */

LUI  R8, 0
ORI  R8, R8, 0xA000    /* Physical page number */
ORI  R8, R8, 0x0F      /* Flags: U|X|W|V */

SW   R8, 0(R7)         /* page_table[0] = PTE */

/* Enable paging */
ENABLE_PAGING
```

### 6.7 Page Faults

When address translation fails in user mode, interrupt 8 (PAGE_FAULT) is raised.

**Causes of page faults:**
- Invalid page (V bit not set)
- Page not accessible to user (U bit not set)
- Write to read-only page (W bit not set)
- Execute on non-executable page (X bit not set)
- Page number out of bounds

**Page fault handler receives:**
- Faulting virtual address (can be computed from saved PC and instruction)
- Interrupt handler in kernel mode can fix the issue and retry

### 6.8 Memory Layout with Protection

**Physical memory layout:**
```
0x00000000 - 0x000007FF : IVT (2KB)
0x00000800 - 0x00000FFF : Reserved
0x00001000 - 0x00008FFF : Kernel code and data
0x00009000 - 0x00009FFF : Page table (4KB for 4MB address space)
0x0000A000 - 0x???????? : User physical pages
```

**Virtual memory layout (user view with paging):**
```
0x00000000 - 0x00000FFF : User code page
0x00001000 - 0x00001FFF : User data page
0x00002000 - 0x00002FFF : User stack page
0x00003000 - 0x003FFFFF : Unmapped (page faults)
```

### 6.9 Protection Scenarios

**Scenario 1: Simple embedded (no protection)**
```c
kernel_mode = 1;
paging_enabled = 0;
/* Everything runs in kernel mode with physical addressing */
```

**Scenario 2: Privilege separation only**
```c
kernel_mode = 0;        /* User mode */
paging_enabled = 0;     /* No paging */
/* User cannot use privileged instructions but has physical memory access */
```

**Scenario 3: Full memory protection**
```c
kernel_mode = 0;        /* User mode */
paging_enabled = 1;     /* Paging on */
page_table_base = 0x9000;
num_pages = 1024;
/* User has virtual addressing with permission checks */
```

---

## 7. Interrupt System

CRISP-32 uses a simple software interrupt system optimized for VM implementation.

### 6.1 Interrupt Vector Table (IVT)

The IVT is located at the beginning of memory (address 0x00000000) and contains up to 256 interrupt handler addresses.

**IVT Entry Format** (8 bytes per entry):
```
Offset +0-3: Handler address (32 bits, little-endian)
Offset +4-7: Reserved (32 bits)
```

**IVT Entry Address Calculation**:
```
IVT_entry_address = 0x00000000 + (interrupt_number × 8)
```

### 7.2 Standard Interrupt Numbers

| Number | Name | Description |
|--------|------|-------------|
| 0 | RESET | Reset/startup (optional) |
| 1 | ILLEGAL_OP | Illegal instruction executed |
| 2 | MEM_FAULT | Memory access violation |
| 3 | DIVIDE_ZERO | Division by zero |
| 4 | SYSCALL | System call (SYSCALL instruction) |
| 5 | BREAK | Breakpoint (BREAK instruction) |
| 6-7 | Reserved | Reserved for future exceptions |
| 7 | PRIVILEGE_VIOLATION | Privileged instruction in user mode |
| 8 | PAGE_FAULT | Page fault (paging enabled) |
| 9-15 | Reserved | Reserved for system use |
| 16 | TIMER | Timer interrupt (external) |
| 17-31 | Reserved | Reserved for system use |
| 32-255 | USER | Available for user/application use |

### 7.3 Interrupt Behavior

**When an interrupt is raised:**
1. Switch to kernel mode (`kernel_mode = 1`)
2. Current PC is saved to `vm->interrupts.saved_pc` (VM structure)
3. All registers R0-R31 are auto-saved to the guest stack (128 bytes)
4. Stack pointer (R29) is decreased by 128 bytes
5. Address where registers were saved is stored in `vm->interrupts.saved_regs_addr`
6. Global interrupt enable flag is cleared (interrupts disabled)
7. Interrupt number is placed in R4 (a0)
8. Pending interrupt bit is cleared
9. PC is set to handler address read from IVT in guest memory
10. Execution continues at handler (now in kernel mode)

**Stack layout after interrupt dispatch:**
```
[R29] →  [R0  ] ← 128 bytes allocated
         [R1  ]
         [R2  ]
         [R3  ]
         ...
         [R30 ]
         [R31 ]
         [previous stack data...]
```

**IVT Access:**
- IVT entries accessed via byte offset calculation: `0x00000000 + (interrupt_num × 8)`
- Handler address read using `c32_read_word(vm->memory + offset)`
- No struct pointers used - direct byte-level access ensures portability

**Handler responsibilities:**
- Perform interrupt-specific work
- Can freely use any registers (all auto-saved)
- Runs in kernel mode with full privileges
- Execute IRET to return

**IRET instruction behavior:**
1. Restore all registers R0-R31 from guest memory (at saved_regs_addr)
2. Restore PC from `vm->interrupts.saved_pc`
3. Stack pointer (R29) automatically restored to pre-interrupt value
4. Restore privilege level (return to user if interrupted user code)
5. Set global interrupt enable flag (interrupts enabled)
6. Continue execution at saved PC

### 6.4 Interrupt Priority

Interrupts are checked in order from 0 to 255. Lower numbers have higher priority. If multiple interrupts are pending, the lowest-numbered interrupt is serviced first.

### 6.5 Nested Interrupts

By default, interrupts are disabled when an interrupt handler is entered (global interrupt enable flag is cleared). To enable nested interrupts:
- Execute EI instruction within the handler
- Higher-priority interrupts can then preempt the current handler
- Each interrupt saves its own complete context (all registers) to the stack
- Stack grows downward with each nested interrupt

**Example of nested interrupt:**
```
Initial state:     [R29 = 0x10000]
                   Stack: [data...]

Interrupt 16:      [R29 = 0x10000 - 128 = 0x0FF80]
                   Stack: [R0-R31 for int 16][data...]

EI executed

Interrupt 5:       [R29 = 0x0FF80 - 128 = 0x0FF00]
                   Stack: [R0-R31 for int 5][R0-R31 for int 16][data...]

IRET from int 5:   [R29 = 0x0FF80]
                   Stack: [R0-R31 for int 16][data...]

IRET from int 16:  [R29 = 0x10000]
                   Stack: [data...]
```

### 6.6 Software Interrupts

Programs can raise software interrupts using the RAISE instruction:
```
RAISE int_num    ; Set interrupt pending bit for int_num
```

This is useful for:
- Inter-task communication
- Deferred processing
- Software exceptions

### 6.7 Synchronous Exceptions

Certain error conditions automatically raise interrupts:
- **Illegal instruction** → Interrupt 1
- **Memory access violation** → Interrupt 2
- **Division by zero** → Interrupt 3 (optional behavior)
- **SYSCALL instruction** → Interrupt 4
- **BREAK instruction** → Interrupt 5

If no handler is installed, the VM stops execution.

### 6.8 External Interrupts

External devices or VM host code can raise interrupts by calling:
```c
c32_raise_interrupt(vm, interrupt_number);
```

Common use cases:
- Timer interrupts (typically interrupt 16)
- I/O device completion
- External events from host system

### 6.9 Interrupt Handler Example

```assembly
; Timer interrupt handler
; All registers (R0-R31) are already auto-saved to stack
; R4 contains interrupt number (16 for timer)
; No manual register saving needed!

timer_handler:
    ; Can freely use any registers
    LW   R5, tick_count     ; Load counter
    ADDI R5, R5, 1          ; Increment
    SW   R5, tick_count     ; Store back
    
    ; Check if we need to do something every 100 ticks
    LUI  R6, 0
    ORI  R6, R6, 100
    REMU R7, R5, R6         ; R7 = tick_count % 100
    BNE  R7, R0, done       ; if not zero, skip
    
    ; Every 100 ticks, do something
    ; ... your code here ...
    
done:
    ; Return (restores ALL registers + PC, enables interrupts)
    IRET

tick_count: .word 0
```

### 6.10 Interrupt Setup Example

```c
/* Setup interrupt handlers in C */
void setup_system(c32_vm_t *vm) {
    /* IVT is at address 0x00000000 in guest memory */
    /* Each entry is 8 bytes: [handler_addr][reserved] */
    
    /* Set timer interrupt handler at address 0x2000 */
    /* IVT entry offset = 16 * 8 = 128 = 0x80 */
    c32_set_interrupt_handler(vm, 16, 0x2000);
    
    /* Set illegal instruction handler */
    /* IVT entry offset = 1 * 8 = 8 = 0x08 */
    c32_set_interrupt_handler(vm, 1, 0x2100);
    
    /* Set divide-by-zero handler */
    /* IVT entry offset = 3 * 8 = 24 = 0x18 */
    c32_set_interrupt_handler(vm, 3, 0x2200);
    
    /* Enable global interrupts */
    vm->interrupts.enabled = 1;
}

/* Implementation of c32_set_interrupt_handler */
void c32_set_interrupt_handler(c32_vm_t *vm, uint8_t int_num, uint32_t handler_addr) {
    uint32_t ivt_offset;
    
    if (int_num >= 256) {
        return;
    }
    
    /* Calculate byte offset in guest memory */
    ivt_offset = 0x00000000 + (int_num * 8);
    
    /* Write handler address to guest memory (little-endian) */
    if (ivt_offset + 4 <= vm->memory_size) {
        c32_write_word(vm->memory + ivt_offset, handler_addr);
    }
}

/* Raise timer interrupt periodically */
void timer_tick(c32_vm_t *vm) {
    c32_raise_interrupt(vm, 16);
}
```

**Guest code can also setup IVT:**
```assembly
; Guest program setting up its own IVT entries
setup_ivt:
    ; Setup timer handler (interrupt 16)
    ; IVT entry address = 0 + (16 * 8) = 128 = 0x80
    LUI  R5, 0
    ORI  R5, R5, 0x80           ; R5 = IVT entry address
    
    LUI  R6, 0
    ORI  R6, R6, 0x2000         ; R6 = handler address
    
    SW   R6, 0(R5)              ; Write to IVT
    
    ; Enable interrupts
    EI
    
    JR   R31
```

---

## 1. Overview

CRISP-32 (C32) is a clean, simple RISC instruction set architecture designed for maximum portability across embedded systems. It is implemented as a virtual machine written in freestanding C89, serving as a hypervisor that provides 100% code portability without requiring a host operating system.

### Design Goals
- Simple, uniform instruction encoding
- Minimal special registers
- No delay slots
- Freestanding implementation (no libc dependencies)
- Complete portability across architectures
- Easy to implement in hardware or software

---

## 2. Architecture Overview

### 2.1 Data Formats
- **Word**: 32 bits (4 bytes)
- **Halfword**: 16 bits (2 bytes)
- **Byte**: 8 bits (1 byte)
- **Instruction**: 64 bits (8 bytes)

### 2.2 Byte Order
- **Little Endian**: All multi-byte values are stored least-significant byte first
- VM implementation handles endianness conversion on big-endian hosts

### 2.3 Registers
- **32 General Purpose Registers**: R0-R31
  - **R0**: Hardwired to zero (reads as 0, writes ignored)
  - **R1-R30**: General purpose
  - **R31**: Link register (return address, used by JAL)
- **Program Counter (PC)**: 32-bit, must be 8-byte aligned

### 2.4 Memory
- Byte-addressable 32-bit address space (4GB maximum)
- Load-store architecture
- No memory-mapped special registers
- Alignment requirements:
  - Word access: 4-byte aligned
  - Halfword access: 2-byte aligned
  - Byte access: No alignment required

---

## 3. Instruction Format

All CRISP-32 instructions are **64 bits (8 bytes)** with a uniform encoding:

```
Bits:  63-56  55-48  47-40  39-32  31-0
      [opcode][ rs  ][ rt  ][ rd  ][immediate]
       8 bits  8 bits 8 bits 8 bits  32 bits
```

### Byte Layout in Memory (Little Endian)
```
Offset:  +0      +1    +2    +3    +4  +5  +6  +7
        [opcode][ rs ][ rt ][ rd ][  immediate   ]
```

### Field Descriptions
- **opcode** (8 bits): Operation code (256 possible instructions)
- **rs** (8 bits): Source register 1 (currently uses 5 bits: 0-31)
- **rt** (8 bits): Source register 2 or destination for I-type ops
- **rd** (8 bits): Destination register for R-type ops
- **immediate** (32 bits): Immediate value, offset, or address

---

## 4. Instruction Set

### 4.1 Arithmetic Operations

#### Register-Register
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| ADD      | 0x01   | rd, rs, rt | rd = rs + rt | Add (signed) |
| ADDU     | 0x02   | rd, rs, rt | rd = rs + rt | Add unsigned |
| SUB      | 0x03   | rd, rs, rt | rd = rs - rt | Subtract (signed) |
| SUBU     | 0x04   | rd, rs, rt | rd = rs - rt | Subtract unsigned |

#### Register-Immediate
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| ADDI     | 0x05   | rt, rs, imm | rt = rs + imm | Add immediate |
| ADDIU    | 0x06   | rt, rs, imm | rt = rs + imm | Add immediate unsigned |

### 4.2 Logical Operations

#### Register-Register
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| AND      | 0x10   | rd, rs, rt | rd = rs & rt | Bitwise AND |
| OR       | 0x11   | rd, rs, rt | rd = rs \| rt | Bitwise OR |
| XOR      | 0x12   | rd, rs, rt | rd = rs ^ rt | Bitwise XOR |
| NOR      | 0x13   | rd, rs, rt | rd = ~(rs \| rt) | Bitwise NOR |

#### Register-Immediate
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| ANDI     | 0x14   | rt, rs, imm | rt = rs & imm | AND immediate |
| ORI      | 0x15   | rt, rs, imm | rt = rs \| imm | OR immediate |
| XORI     | 0x16   | rt, rs, imm | rt = rs ^ imm | XOR immediate |
| LUI      | 0x17   | rt, imm | rt = imm << 16 | Load upper immediate |

### 4.3 Shift Operations

#### Immediate Shift Amount
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| SLL      | 0x20   | rd, rt, imm | rd = rt << imm | Shift left logical |
| SRL      | 0x21   | rd, rt, imm | rd = rt >> imm | Shift right logical |
| SRA      | 0x22   | rd, rt, imm | rd = rt >> imm | Shift right arithmetic |

#### Variable Shift Amount
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| SLLV     | 0x23   | rd, rt, rs | rd = rt << rs | Shift left logical variable |
| SRLV     | 0x24   | rd, rt, rs | rd = rt >> rs | Shift right logical variable |
| SRAV     | 0x25   | rd, rt, rs | rd = rt >> rs | Shift right arithmetic variable |

### 4.4 Comparison Operations

#### Register-Register
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| SLT      | 0x30   | rd, rs, rt | rd = (rs < rt) ? 1 : 0 | Set less than (signed) |
| SLTU     | 0x31   | rd, rs, rt | rd = (rs < rt) ? 1 : 0 | Set less than unsigned |

#### Register-Immediate
| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| SLTI     | 0x32   | rt, rs, imm | rt = (rs < imm) ? 1 : 0 | Set less than immediate |
| SLTIU    | 0x33   | rt, rs, imm | rt = (rs < imm) ? 1 : 0 | Set less than immediate unsigned |

### 4.5 Multiplication and Division

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| MUL      | 0x40   | rd, rs, rt | rd = (rs * rt)[31:0] | Multiply (lower 32 bits) |
| MULH     | 0x41   | rd, rs, rt | rd = (rs * rt)[63:32] | Multiply high (signed) |
| MULHU    | 0x42   | rd, rs, rt | rd = (rs * rt)[63:32] | Multiply high unsigned |
| DIV      | 0x43   | rd, rs, rt | rd = rs / rt | Divide (signed) |
| DIVU     | 0x44   | rd, rs, rt | rd = rs / rt | Divide unsigned |
| REM      | 0x45   | rd, rs, rt | rd = rs % rt | Remainder (signed) |
| REMU     | 0x46   | rd, rs, rt | rd = rs % rt | Remainder unsigned |

**Note**: Division by zero sets rd to 0 (no exception).

### 4.6 Load Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| LW       | 0x50   | rt, offset(rs) | rt = mem[rs+offset] | Load word |
| LH       | 0x51   | rt, offset(rs) | rt = sign_ext(mem[rs+offset]) | Load halfword (signed) |
| LHU      | 0x52   | rt, offset(rs) | rt = zero_ext(mem[rs+offset]) | Load halfword unsigned |
| LB       | 0x53   | rt, offset(rs) | rt = sign_ext(mem[rs+offset]) | Load byte (signed) |
| LBU      | 0x54   | rt, offset(rs) | rt = zero_ext(mem[rs+offset]) | Load byte unsigned |

**Offset**: Signed 32-bit immediate value added to base address in rs.

### 4.7 Store Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| SW       | 0x58   | rt, offset(rs) | mem[rs+offset] = rt | Store word |
| SH       | 0x59   | rt, offset(rs) | mem[rs+offset] = rt[15:0] | Store halfword |
| SB       | 0x5A   | rt, offset(rs) | mem[rs+offset] = rt[7:0] | Store byte |

### 4.8 Branch Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| BEQ      | 0x60   | rs, rt, offset | if (rs == rt) pc += offset | Branch if equal |
| BNE      | 0x61   | rs, rt, offset | if (rs != rt) pc += offset | Branch if not equal |
| BLEZ     | 0x62   | rs, offset | if (rs <= 0) pc += offset | Branch if ≤ zero |
| BGTZ     | 0x63   | rs, offset | if (rs > 0) pc += offset | Branch if > zero |
| BLTZ     | 0x64   | rs, offset | if (rs < 0) pc += offset | Branch if < zero |
| BGEZ     | 0x65   | rs, offset | if (rs >= 0) pc += offset | Branch if ≥ zero |

**Offset**: Signed 32-bit immediate, byte offset (not instruction offset).
**No delay slots**: Branch takes effect immediately.

### 4.9 Jump Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| J        | 0x70   | target | pc = target | Jump |
| JAL      | 0x71   | target | r31 = pc+8; pc = target | Jump and link |
| JR       | 0x72   | rs | pc = rs | Jump register |
| JALR     | 0x73   | rd, rs | rd = pc+8; pc = rs | Jump and link register |

**Target**: 32-bit absolute address in immediate field.
**Return address**: Points to instruction after delay slot would be (PC+8).

### 4.10 System Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| SYSCALL  | 0xF0   | code | system call | System call (raises interrupt 4) |
| BREAK    | 0xF1   | code | breakpoint | Breakpoint (raises interrupt 5) |
| NOP      | 0x00   | - | no operation | No operation |

### 4.11 Interrupt Control Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| EI       | 0xF2   | - | Enable interrupts | Set global interrupt enable flag (privileged) |
| DI       | 0xF3   | - | Disable interrupts | Clear global interrupt enable flag (privileged) |
| IRET     | 0xF4   | - | Return from interrupt | Restore PC and context, enable interrupts (privileged) |
| RAISE    | 0xF5   | int_num | Raise software interrupt | Set interrupt pending bit |
| GETPC    | 0xF6   | rd | rd = saved_pc | Get saved PC from interrupt |

### 4.12 Privilege and MMU Control Operations

| Mnemonic | Opcode | Format | Operation | Description |
|----------|--------|--------|-----------|-------------|
| ENABLE_PAGING  | 0xF7 | - | Enable virtual addressing | Turn on MMU (privileged) |
| DISABLE_PAGING | 0xF8 | - | Disable virtual addressing | Turn off MMU (privileged) |
| SET_PTBR | 0xF9 | rd, rt | Set page table | rd=base, rt=num_pages (privileged) |
| ENTER_USER | 0xFB | - | Switch to user mode | Drop privilege to user (privileged) |
| GETMODE | 0xFC | rd | rd = kernel_mode | Get current privilege level (1=kernel, 0=user) |

---

## 5. VM Implementation Requirements

### 5.1 Freestanding Environment
- **No standard library**: Cannot use libc functions
- **No system calls**: Cannot use OS services
- **No dynamic allocation**: Memory provided externally
- **Allowed headers**: Only basic type headers (stdint.h equivalent)

### 5.2 Required Built-in Functions
```c
c32_memcpy()   - Memory copy
c32_memset()   - Memory set
c32_memcmp()   - Memory compare
c32_strlen()   - String length
c32_strcpy()   - String copy
c32_strcmp()   - String compare
```

### 5.3 Endianness Handling
- VM must detect host endianness at compile time
- Use `-DC32_HOST_BIG_ENDIAN` flag when compiling for big-endian hosts
- All memory access functions handle conversion automatically

### 5.4 VM State Structure
```c
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
```

**Important Notes:**
- **Privilege and paging state**: Stored in VM structure, not directly accessible to guest
- **Current registers and PC**: Fast access in VM structure
- **Saved registers**: Written to guest memory stack during interrupts
- **Page table**: Lives in guest memory at `page_table_base`
- **IVT**: Lives in guest memory at address 0x00000000

### 5.5 Execution Cycle
1. Check for pending interrupts (if enabled)
2. Check PC alignment (8-byte boundary)
3. Check PC bounds
4. Fetch instruction (8 bytes from memory at PC)
5. Decode instruction fields
6. Execute operation
7. Update PC (PC += 8 or branch/jump target)
8. Enforce R0 = 0

---

## 8. Pseudo-Instructions

These are assembler conveniences, not real instructions:

| Pseudo | Real Instruction | Description |
|--------|------------------|-------------|
| MOV rd, rs | OR rd, rs, R0 | Move register |
| LI rt, imm | ORI rt, R0, imm | Load immediate (small) |
| LA rt, addr | LUI rt, addr[31:16]<br>ORI rt, rt, addr[15:0] | Load address |
| NOT rd, rs | NOR rd, rs, R0 | Bitwise NOT |
| B offset | BEQ R0, R0, offset | Unconditional branch |
| BAL offset | BGEZ R0, offset | Branch and link (using R31) |

---

## 9. Register Usage Conventions

These are **software conventions only**, not enforced by hardware:

| Register | Name | Usage | Preserved? |
|----------|------|-------|------------|
| R0 | zero | Constant zero | N/A |
| R1 | at | Assembler temporary | No |
| R2-R3 | v0-v1 | Return values | No |
| R4-R7 | a0-a3 | Function arguments | No |
| R8-R15 | t0-t7 | Temporaries | No |
| R16-R23 | s0-s7 | Saved registers | Yes |
| R24-R25 | t8-t9 | More temporaries | No |
| R26-R27 | k0-k1 | Kernel reserved | No |
| R28 | gp | Global pointer (optional) | Yes |
| R29 | sp | Stack pointer (optional) | Yes |
| R30 | fp | Frame pointer (optional) | Yes |
| R31 | ra | Return address | No |

---

## 10. Memory Model

### 8.1 Address Space
- 32-bit byte-addressable (0x00000000 - 0xFFFFFFFF)
- No memory protection (bare metal)
- No virtual memory

### 8.2 Suggested Memory Layout
```
0x00000000 - 0x000000FF : Exception vectors (reserved)
0x00000100 - 0x00000FFF : System reserved
0x00001000 - 0x???????? : Code segment (.text)
0x???????? - 0x???????? : Read-only data (.rodata)
0x???????? - 0x???????? : Data segment (.data)
0x???????? - 0x???????? : BSS segment (.bss)
0x???????? - 0xFFFFFFFF : Heap and Stack
```

### 8.3 Stack Convention
- Grows downward (towards lower addresses)
- Stack pointer (R29) points to last used location
- Caller cleans up arguments

---

## 9. Error Handling

The VM stops execution (sets `running = 0`) on:
- Invalid opcode
- PC misalignment (not 8-byte aligned)
- PC out of bounds
- Memory access out of bounds
- SYSCALL instruction
- BREAK instruction

**Note**: Division by zero does NOT stop execution; it returns 0.

---

## 12. Binary Format

CRISP-32 binaries are raw memory images:
- Instructions stored in little-endian format
- No file headers or metadata
- Loaded directly into VM memory at specified offset
- Entry point must be set in PC after loading

---

## 13. Design Rationale

### Why 64-bit instructions?
- Eliminates need for function codes
- Allows 32-bit immediates (load any constant in one instruction)
- Simplifies decode (byte-aligned fields)
- More opcode space (256 vs 64)
- Future expandable to 256 registers

### Why no delay slots?
- Simpler VM implementation
- Easier to understand and debug
- Not necessary for software implementation

### Why little-endian?
- Matches most modern architectures
- Natural for x86/ARM/RISC-V compatibility
- Simplifies multi-byte operations

### Why minimal special registers?
- Maximum flexibility for programmer
- Simpler hardware/software implementation
- Software conventions handle calling conventions

### Why simple interrupts?
- VM-only design allows simpler implementation than hardware
- Table-based dispatch is fast and easy to understand
- Minimal context save (just PC + 4 registers)
- Priority-based system is straightforward
- No complex interrupt controllers needed

### Why two privilege levels?
- Sufficient for most embedded use cases
- Simple to implement and understand
- Clear security boundary
- Can run without privilege separation if not needed

### Why simple paging?
- Flat page table is easy to understand and implement
- No multi-level translation overhead
- Kernel bypasses paging for performance
- Optional feature - can run without it
- Page table visible in guest memory for debugging

---

## 14. Future Extensions

Potential additions while maintaining compatibility:
- Floating-point instructions (opcodes 0x80-0x8F)
- Atomic operations (opcodes 0x90-0x9F)
- Vector/SIMD instructions (opcodes 0xA0-0xAF)
- Extend register file to 256 registers (use full 8-bit rs/rt/rd)
- TLB caching for paging performance
- Additional privilege levels (opcodes 0xB0-0xBF)
- Hardware I/O instructions (opcodes 0xC0-0xCF)

---

## Appendix A: Complete Opcode Map

```
0x00: NOP        0x20: SLL        0x40: MUL        0x60: BEQ
0x01: ADD        0x21: SRL        0x41: MULH       0x61: BNE
0x02: ADDU       0x22: SRA        0x42: MULHU      0x62: BLEZ
0x03: SUB        0x23: SLLV       0x43: DIV        0x63: BGTZ
0x04: SUBU       0x24: SRLV       0x44: DIVU       0x64: BLTZ
0x05: ADDI       0x25: SRAV       0x45: REM        0x65: BGEZ
0x06: ADDIU      0x26-0x2F: Reserved   0x46: REMU  0x66-0x6F: Reserved
0x07-0x0F: Reserved   0x30: SLT   0x47-0x4F: Reserved   0x70: J
0x10: AND        0x31: SLTU       0x50: LW         0x71: JAL
0x11: OR         0x32: SLTI       0x51: LH         0x72: JR
0x12: XOR        0x33: SLTIU      0x52: LHU        0x73: JALR
0x13: NOR        0x34-0x3F: Reserved   0x53: LB    0x74-0xEF: Reserved
0x14: ANDI       0x54: LBU        0xF0: SYSCALL
0x15: ORI        0x55-0x57: Reserved   0xF1: BREAK
0x16: XORI       0x58: SW         0xF2: EI
0x17: LUI        0x59: SH         0xF3: DI
0x18-0x1F: Reserved   0x5A: SB    0xF4: IRET
                 0x5B-0x5F: Reserved   0xF5: RAISE
                                  0xF6: GETPC
                                  0xF7: ENABLE_PAGING
                                  0xF8: DISABLE_PAGING
                                  0xF9: SET_PTBR
                                  0xFA: Reserved
                                  0xFB: ENTER_USER
                                  0xFC: GETMODE
                                  0xFD-0xFF: Reserved
```

---

## Appendix B: Instruction Encoding Examples

### Example 1: ADD R5, R10, R12
```
Opcode: 0x01 (ADD)
rs: 0x0A (R10)
rt: 0x0C (R12)
rd: 0x05 (R5)
immediate: 0x00000000 (unused)

Bytes: 01 0A 0C 05 00 00 00 00
```

### Example 2: ADDI R3, R17, 42
```
Opcode: 0x05 (ADDI)
rs: 0x11 (R17)
rt: 0x03 (R3)
rd: 0x00 (unused)
immediate: 0x0000002A (42)

Bytes: 05 11 03 00 2A 00 00 00
```

### Example 3: LW R8, 100(R2)
```
Opcode: 0x50 (LW)
rs: 0x02 (R2 - base address)
rt: 0x08 (R8 - destination)
rd: 0x00 (unused)
immediate: 0x00000064 (100)

Bytes: 50 02 08 00 64 00 00 00
```

### Example 4: JAL 0x1000
```
Opcode: 0x71 (JAL)
rs: 0x00 (unused)
rt: 0x00 (unused)
rd: 0x00 (unused)
immediate: 0x00001000 (target address)

Bytes: 71 00 00 00 00 10 00 00
```

---

**End of Specification**