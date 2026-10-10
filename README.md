# IITPSx Processor: Two-Pass Assembler & Emulator
**CS2102 Mini Project**  
**Name:** Yashashwani | **ID:** 2501AI49  
*"I declare this is my own work."*

---

## 1. Overview
This project provides a complete software stack for the custom **IITPSx** processor:
1. **Assembler (`asm`)**: A robust, table-driven, two-pass assembler implemented with a single unified pass routine that produces raw binary object files (`.o`) and advanced formatted listing files (`.lst`).
2. **Emulator (`emu`)**: An instruction-accurate simulator featuring step tracing (`-trace`), post-run memory dumping (`-dump`), configurable instruction limits (`-max N`), and comprehensive runtime error detection.
3. **Test Suite (`tests/`)**: A collection of 10 test programs (`test01.asm` through `test10.asm`) accompanied by listing files (`.lst`) and execution logs (`.log`).

---

## 2. Processor Architecture & Specifications
- **Word Size:** 32-bit words, word-addressed memory starting at address `0`.
- **Memory Capacity:** 65,536 words (addresses `0x0000` to `0xFFFF`).
- **Registers (32-bit):**
  - `A`: Primary accumulator register.
  - `B`: Secondary internal stack register.
  - `PC`: Program Counter (word address).
  - `SP`: Stack Pointer (word address, initialized to top of memory `65535`).
- **Instruction Format:** One 32-bit word:
  - Bottom 8 bits: `opcode` (`word & 0xFF`).
  - Upper 24 bits: Signed 2's complement operand (`word >> 8` with sign extension).
  - Machine encoding: `word = ((operand & 0xFFFFFF) << 8) | opcode`.
- **Execution Rule:** `PC` is incremented **BEFORE** the instruction's operation executes.

### Instruction Set Architecture (ISA)

| Mnemonic | Opcode | Operand | Description / Semantics |
| :--- | :---: | :---: | :--- |
| `ldc` | 0 | value | $B := A; \quad A := \text{value}$ |
| `adc` | 1 | value | $A := A + \text{value}$ |
| `ldl` | 2 | offset | $B := A; \quad A := \text{mem}[SP + \text{offset}]$ |
| `stl` | 3 | offset | $\text{mem}[SP + \text{offset}] := A; \quad A := B$ |
| `ldnl` | 4 | offset | $A := \text{mem}[A + \text{offset}]$ |
| `stnl` | 5 | offset | $\text{mem}[A + \text{offset}] := B$ |
| `add` | 6 | - | $A := B + A$ |
| `sub` | 7 | - | $A := B - A$ |
| `shl` | 8 | - | $A := B \ll A$ |
| `shr` | 9 | - | $A := B \gg A$ (arithmetic right shift) |
| `adj` | 10 | value | $SP := SP + \text{value}$ |
| `a2sp` | 11 | - | $SP := A; \quad A := B$ |
| `sp2a` | 12 | - | $B := A; \quad A := SP$ |
| `call` | 13 | offset | $B := A; \quad A := PC; \quad PC := PC + \text{offset}$ |
| `return` | 14 | - | $PC := A; \quad A := B$ |
| `brz` | 15 | offset | if $A == 0$ then $PC := PC + \text{offset}$ |
| `brlz` | 16 | offset | if $A < 0$ then $PC := PC + \text{offset}$ |
| `br` | 17 | offset | $PC := PC + \text{offset}$ |
| `HALT` | 18 | - | Terminate emulator execution cleanly |
| `mul` | 19 | - | $A := B \times A$ |
| `div` | 20 | - | if $A == 0$ runtime error; else $A := B / A$ (signed 32-bit) |
| `data` | - | value | Reserves one word holding raw 32-bit value |
| `SET` | - | value | Pseudo-op: sets label to value instead of PC; emits no code |

---

## 3. Build & Run Instructions

### Prerequisites
- GCC / G++ supporting C++17.
- Linux operating system.

### Compilation
The provided `Makefile` compiles all binaries warning-free with strict flags:
```bash
g++ -std=c++17 -pedantic -Wall -Wextra -Wpointer-arith -Wwrite-strings -O2
```

Commands:
- `make` or `make all`: Compiles both `asm` and `emu`.
- `make asm`: Compiles the assembler (`asm.cpp` $\rightarrow$ `asm`).
- `make emu`: Compiles the emulator (`emu.cpp` $\rightarrow$ `emu`).
- `make test`: Runs all 10 automated test suites, verifies outputs, and updates `.lst` and `.log` files.
- `make clean`: Removes binaries, object files (`*.o`), listings (`*.lst`), and logs (`*.log`).

### Running the Assembler
```bash
./asm <source.asm>
```
- Produces `<source.o>` (binary object file) and `<source.lst>` (advanced listing).
- Prints warnings and error diagnostics to `stderr` with line numbers and descriptive messages.
- If any error occurs, no `.o` file is generated.

### Running the Emulator
```bash
./emu <program.o> [options]
```
Options:
- `-trace`: Prints PC, SP, A, B, and the decoded instruction before each execution cycle.
- `-dump`: Prints non-zero memory contents in hex (4 words per line) upon completion.
- `-max N`: Sets maximum executed instruction limit (default: 1,000,000).

---

## 4. Key Design Decisions & Architectural Highlights

### 1. Table-Driven Assembler
All instructions, pseudo-ops, opcodes, and operand types are defined in a single immutable table:
```cpp
struct InstructionInfo {
    const char* mnemonic;
    int opcode;
    OperandType opType;
};
```
No instruction-specific logic is hardcoded outside this table.

### 2. Single Unified Two-Pass Routine
Both passes are executed via the exact same function:
```cpp
bool assemblePass(const std::vector<ParsedLine>& lines, Pass pass, ...);
```
- **Pass 1 (`Pass::PASS_1`):** Builds the symbol table, processes `SET` directives, validates line syntax and label rules, and computes addresses. Forward references to undefined labels do not raise errors.
- **Pass 2 (`Pass::PASS_2`):** Resolves symbols, computes PC-relative branch offsets (`target - (PC + 1)`), performs 24-bit range checks, records label usage, flags unused labels, and emits 32-bit machine words.

### 3. Number and Label Tokenization
- Numeric parsing uses `std::strtol(..., &endPtr, 0)` with base 0 to automatically support decimal, hexadecimal (`0x...`), and octal (`0...`).
- Checks that `*endPtr == '\0'` to ensure entire token consumption (rejecting malformed tokens like `08ge` and invalid octal `08`).
- Labels must start with an alphabetic character and contain only alphanumeric characters (`[a-zA-Z][a-zA-Z0-9]*`). Underscores and non-alphanumeric characters are strictly rejected as bogus label names.

### 4. Advanced Listing Format
Listing files format each instruction word as:
```
AAAAAAAA XXXXXXXX mnemonic operand
```
Preceded by `AAAAAAAA label:` lines for any labels defined at that address. Branch instructions print the symbolic label name if present at the target address.

---

## 5. Explicit Assumptions (As Required by Specification)

1. **Case-Insensitive Mnemonics:**
   Mnemonics (`HALT`, `halt`, `SET`, `set`, `ldc`, `LDC`, etc.) are recognized case-insensitively by converting tokens during lookup.
2. **Label Character Set:**
   Labels must strictly adhere to the specification: starts with an alphabetic character (`a-z`, `A-Z`) followed only by alphanumeric characters (`a-z`, `A-Z`, `0-9`). Characters such as `_` are rejected as `bogus label name`.
3. **Top of Memory & Initial SP:**
   With 65,536 memory words (addresses `0x0000` to `0xFFFF`), `SP` is initialized to address `65535` (`0x0000FFFF`), which represents the top accessible memory cell.
4. **Operand Ranges:**
   - Instruction operands must fit within the signed 24-bit range: $[-8388608, 8388607]$.
   - `data` operands represent raw 32-bit words.
5. **SET Pseudo-Operation:**
   Does not generate machine code and does not advance the address counter (`PC`). Labels defined with `SET` can be referenced both before and after the `SET` statement.
6. **Integer Division Overflow Guard:**
   Signed 32-bit division `INT32_MIN / -1` causes a hardware exception on x86 processors. The emulator explicitly guards against this condition and sets `A = INT32_MIN`.
7. **Empty / Blank / Comment-Only Lines:**
   Lines that are empty, whitespace-only, or contain only a comment (`; ...`) after trimming are ignored and are not treated as errors. Label-only lines (e.g. `loop:`) define the label at the current `PC` without emitting code.

---

## 6. Test Suite Description & Results

| Test File | Description | Expected Outcome | Status |
| :--- | :--- | :--- | :---: |
| `tests/test01.asm` | Specification verification test (immediate values, branches, data) | Clean assembly; exact byte match with spec | **PASS** |
| `tests/test02.asm` | Comprehensive error test suite (bogus label, duplicate label, invalid numbers, unexpected operands, missing operands, etc.) | All 9 errors reported; no `.o` file | **PASS** |
| `tests/test03.asm` | `SET` pseudo-op test with forward and backward references | $A = 75 + 66 = 141$ (`0x8D`) | **PASS** |
| `tests/test04.asm` | `mul` and `div` test ($6 \times 7 = 42; \quad 42 / 2 = 21$) | $A = 21$ (`0x15`) | **PASS** |
| `tests/test05.asm` | Bubble sort of an array using `ldnl`/`stnl` and `sub` + `brlz` | Array sorted: `{7, 12, 23, 34, 45, 89}` in `-dump` | **PASS** |
| `tests/test06.asm` | Sum of first 10 natural numbers ($1 + 2 + \dots + 10$) | $A = 55$ (`0x37`) | **PASS** |
| `tests/test07.asm` | Polynomial evaluation $P(x) = 3x^2 + 5x + 7$ for $x=4$ | $A = 75$ (`0x4B`) | **PASS** |
| `tests/test08.asm` | Arithmetic mean of 5 array elements using `div` | $A = 35$ (`0x23`) | **PASS** |
| `tests/test09.asm` | Division by zero runtime error test | Emulator reports error and exits code 1 | **PASS** |
| `tests/test10.asm` | Factorial calculation $6! = 6 \times 5 \times 4 \times 3 \times 2 \times 1$ | $A = 720$ (`0x2D0`) | **PASS** |
