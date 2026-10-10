// Name: Yashashwani | ID: 2501AI49
// "I declare this is my own work."

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <climits>
#include <iomanip>

// ============================================================================
// Constants and Configuration
// ============================================================================

// Processor memory: 65,536 32-bit words
constexpr size_t MEMORY_CAPACITY = 65536;

// Default limit on executed instructions to guard against infinite loops
constexpr uint64_t DEFAULT_MAX_INSTRUCTIONS = 1000000ULL;

// Opcode definitions matching IITPSx specification
constexpr uint32_t OP_LDC    = 0;
constexpr uint32_t OP_ADC    = 1;
constexpr uint32_t OP_LDL    = 2;
constexpr uint32_t OP_STL    = 3;
constexpr uint32_t OP_LDNL   = 4;
constexpr uint32_t OP_STNL   = 5;
constexpr uint32_t OP_ADD    = 6;
constexpr uint32_t OP_SUB    = 7;
constexpr uint32_t OP_SHL    = 8;
constexpr uint32_t OP_SHR    = 9;
constexpr uint32_t OP_ADJ    = 10;
constexpr uint32_t OP_A2SP   = 11;
constexpr uint32_t OP_SP2A   = 12;
constexpr uint32_t OP_CALL   = 13;
constexpr uint32_t OP_RETURN = 14;
constexpr uint32_t OP_BRZ    = 15;
constexpr uint32_t OP_BRLZ   = 16;
constexpr uint32_t OP_BR     = 17;
constexpr uint32_t OP_HALT   = 18;
constexpr uint32_t OP_MUL    = 19;
constexpr uint32_t OP_DIV    = 20;

// ============================================================================
// Processor State Structure
// ============================================================================

struct ProcessorState {
    int32_t regA = 0;
    int32_t regB = 0;
    uint32_t regPC = 0;
    uint32_t regSP = static_cast<uint32_t>(MEMORY_CAPACITY - 1); // Top of memory
    std::vector<int32_t> memory;
    uint64_t instructionCount = 0;
    bool halted = false;

    ProcessorState() : memory(MEMORY_CAPACITY, 0) {}
};

// Returns mnemonic string for a given opcode
static const char* getOpcodeName(uint32_t opcode) {
    switch (opcode) {
        case OP_LDC:    return "ldc";
        case OP_ADC:    return "adc";
        case OP_LDL:    return "ldl";
        case OP_STL:    return "stl";
        case OP_LDNL:   return "ldnl";
        case OP_STNL:   return "stnl";
        case OP_ADD:    return "add";
        case OP_SUB:    return "sub";
        case OP_SHL:    return "shl";
        case OP_SHR:    return "shr";
        case OP_ADJ:    return "adj";
        case OP_A2SP:   return "a2sp";
        case OP_SP2A:   return "sp2a";
        case OP_CALL:   return "call";
        case OP_RETURN: return "return";
        case OP_BRZ:    return "brz";
        case OP_BRLZ:   return "brlz";
        case OP_BR:     return "br";
        case OP_HALT:   return "HALT";
        case OP_MUL:    return "mul";
        case OP_DIV:    return "div";
        default:        return "unknown";
    }
}

// Checks if opcode takes an operand
static bool opcodeHasOperand(uint32_t opcode) {
    switch (opcode) {
        case OP_LDC:
        case OP_ADC:
        case OP_LDL:
        case OP_STL:
        case OP_LDNL:
        case OP_STNL:
        case OP_ADJ:
        case OP_CALL:
        case OP_BRZ:
        case OP_BRLZ:
        case OP_BR:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// Binary Loader
// ============================================================================

// Loads binary object file into processor memory starting at address 0
static bool loadBinary(const std::string& filename, ProcessorState& state, size_t& loadedWords) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: File not found: " << filename << std::endl;
        return false;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize < 0) {
        std::cerr << "Error: Could not read file size: " << filename << std::endl;
        return false;
    }

    if (fileSize % 4 != 0) {
        std::cerr << "Error: Bad file size (not a multiple of 4 bytes): " << filename
                  << " (" << fileSize << " bytes)" << std::endl;
        return false;
    }

    size_t words = static_cast<size_t>(fileSize / 4);
    if (words > MEMORY_CAPACITY) {
        std::cerr << "Error: Program exceeds memory capacity: " << words
                  << " words > " << MEMORY_CAPACITY << " words" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        std::cerr << "Error: Failed to read object file: " << filename << std::endl;
        return false;
    }

    for (size_t i = 0; i < words; ++i) {
        uint32_t b0 = buffer[i * 4 + 0];
        uint32_t b1 = buffer[i * 4 + 1];
        uint32_t b2 = buffer[i * 4 + 2];
        uint32_t b3 = buffer[i * 4 + 3];
        uint32_t word = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
        state.memory[i] = static_cast<int32_t>(word);
    }

    loadedWords = words;
    return true;
}

// ============================================================================
// Instruction Execution Engine
// ============================================================================

// Executes a single instruction cycle
static bool executeStep(ProcessorState& state, bool trace) {
    if (state.regPC >= MEMORY_CAPACITY) {
        std::cerr << "Error: PC out of range (PC = " << state.regPC << ")" << std::endl;
        return false;
    }

    uint32_t instructionPC = state.regPC;
    int32_t word = state.memory[instructionPC];

    // Decode opcode (bottom 8 bits) and operand (upper 24 bits sign-extended)
    uint32_t opcode = static_cast<uint32_t>(word) & 0xFFu;
    int32_t operand = word >> 8; // Arithmetic right shift performs 2's complement sign extension

    // PC is incremented BEFORE the instruction's action executes
    state.regPC++;

    if (trace) {
        std::cout << "PC: " << std::hex << std::setfill('0') << std::setw(8) << instructionPC
                  << "  SP: " << std::setw(8) << state.regSP
                  << "  A: " << std::setw(8) << static_cast<uint32_t>(state.regA)
                  << "  B: " << std::setw(8) << static_cast<uint32_t>(state.regB)
                  << "  " << getOpcodeName(opcode);
        if (opcodeHasOperand(opcode)) {
            std::cout << " " << std::dec << operand << " (0x" << std::hex << (operand & 0xFFFFFF) << ")";
        }
        std::cout << std::dec << "\n";
    }

    switch (opcode) {
        case OP_LDC:
            state.regB = state.regA;
            state.regA = operand;
            break;

        case OP_ADC:
            state.regA = state.regA + operand;
            break;

        case OP_LDL: {
            int64_t targetAddr = static_cast<int64_t>(state.regSP) + operand;
            if (targetAddr < 0 || targetAddr >= static_cast<int64_t>(MEMORY_CAPACITY)) {
                std::cerr << "Error: Memory access out of range (address " << targetAddr
                          << " at PC " << instructionPC << ")" << std::endl;
                return false;
            }
            state.regB = state.regA;
            state.regA = state.memory[static_cast<size_t>(targetAddr)];
            break;
        }

        case OP_STL: {
            int64_t targetAddr = static_cast<int64_t>(state.regSP) + operand;
            if (targetAddr < 0 || targetAddr >= static_cast<int64_t>(MEMORY_CAPACITY)) {
                std::cerr << "Error: Memory access out of range (address " << targetAddr
                          << " at PC " << instructionPC << ")" << std::endl;
                return false;
            }
            state.memory[static_cast<size_t>(targetAddr)] = state.regA;
            state.regA = state.regB;
            break;
        }

        case OP_LDNL: {
            int64_t targetAddr = static_cast<int64_t>(state.regA) + operand;
            if (targetAddr < 0 || targetAddr >= static_cast<int64_t>(MEMORY_CAPACITY)) {
                std::cerr << "Error: Memory access out of range (address " << targetAddr
                          << " at PC " << instructionPC << ")" << std::endl;
                return false;
            }
            state.regA = state.memory[static_cast<size_t>(targetAddr)];
            break;
        }

        case OP_STNL: {
            int64_t targetAddr = static_cast<int64_t>(state.regA) + operand;
            if (targetAddr < 0 || targetAddr >= static_cast<int64_t>(MEMORY_CAPACITY)) {
                std::cerr << "Error: Memory access out of range (address " << targetAddr
                          << " at PC " << instructionPC << ")" << std::endl;
                return false;
            }
            state.memory[static_cast<size_t>(targetAddr)] = state.regB;
            break;
        }

        case OP_ADD:
            state.regA = state.regB + state.regA;
            break;

        case OP_SUB:
            state.regA = state.regB - state.regA;
            break;

        case OP_SHL: {
            uint32_t shiftAmount = static_cast<uint32_t>(state.regA) & 31u;
            state.regA = static_cast<int32_t>(static_cast<uint32_t>(state.regB) << shiftAmount);
            break;
        }

        case OP_SHR: {
            uint32_t shiftAmount = static_cast<uint32_t>(state.regA) & 31u;
            state.regA = state.regB >> shiftAmount; // Arithmetic shift in C++ for signed type
            break;
        }

        case OP_ADJ:
            state.regSP = static_cast<uint32_t>(static_cast<int64_t>(state.regSP) + operand);
            break;

        case OP_A2SP:
            state.regSP = static_cast<uint32_t>(state.regA);
            state.regA = state.regB;
            break;

        case OP_SP2A:
            state.regB = state.regA;
            state.regA = static_cast<int32_t>(state.regSP);
            break;

        case OP_CALL:
            state.regB = state.regA;
            state.regA = static_cast<int32_t>(state.regPC);
            state.regPC = static_cast<uint32_t>(static_cast<int64_t>(state.regPC) + operand);
            break;

        case OP_RETURN:
            state.regPC = static_cast<uint32_t>(state.regA);
            state.regA = state.regB;
            break;

        case OP_BRZ:
            if (state.regA == 0) {
                state.regPC = static_cast<uint32_t>(static_cast<int64_t>(state.regPC) + operand);
            }
            break;

        case OP_BRLZ:
            if (state.regA < 0) {
                state.regPC = static_cast<uint32_t>(static_cast<int64_t>(state.regPC) + operand);
            }
            break;

        case OP_BR:
            state.regPC = static_cast<uint32_t>(static_cast<int64_t>(state.regPC) + operand);
            break;

        case OP_HALT:
            state.halted = true;
            break;

        case OP_MUL:
            state.regA = state.regB * state.regA;
            break;

        case OP_DIV:
            if (state.regA == 0) {
                std::cerr << "Error: Division by zero at PC " << instructionPC << std::endl;
                return false;
            }
            if (state.regB == INT32_MIN && state.regA == -1) {
                // Guard against 32-bit signed division overflow
                state.regA = INT32_MIN;
            } else {
                state.regA = state.regB / state.regA;
            }
            break;

        default:
            std::cerr << "Error: Unknown opcode " << opcode << " (0x" << std::hex << opcode
                      << std::dec << ") at PC " << instructionPC << std::endl;
            return false;
    }

    state.instructionCount++;
    return true;
}

// ============================================================================
// Memory Dump Function
// ============================================================================

// Dumps memory contents in hex format (4 words per line) up to highest modified/loaded address
static void dumpMemory(const ProcessorState& state, size_t maxDumpWords) {
    std::cout << "\n========== Memory Dump ==========\n";
    size_t endWord = maxDumpWords;
    if (endWord > MEMORY_CAPACITY) endWord = MEMORY_CAPACITY;

    // Find highest non-zero address to avoid printing 64k lines of zeros
    size_t lastNonZero = 0;
    for (size_t i = 0; i < MEMORY_CAPACITY; ++i) {
        if (state.memory[i] != 0) {
            lastNonZero = i;
        }
    }
    if (lastNonZero + 1 > endWord) {
        endWord = lastNonZero + 1;
    }
    // Round up to multiple of 4
    endWord = ((endWord + 3) / 4) * 4;
    if (endWord > MEMORY_CAPACITY) endWord = MEMORY_CAPACITY;

    for (size_t i = 0; i < endWord; i += 4) {
        std::cout << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << i << ": ";
        for (size_t j = 0; j < 4; ++j) {
            if (i + j < MEMORY_CAPACITY) {
                std::cout << std::setw(8) << static_cast<uint32_t>(state.memory[i + j]) << " ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::dec;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <prog.o> [-trace] [-dump] [-max N]" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    bool trace = false;
    bool dump = false;
    uint64_t maxInstructions = DEFAULT_MAX_INSTRUCTIONS;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-trace") {
            trace = true;
        } else if (arg == "-dump") {
            dump = true;
        } else if (arg == "-max" && i + 1 < argc) {
            char* endPtr = nullptr;
            maxInstructions = std::strtoull(argv[++i], &endPtr, 0);
        } else {
            std::cerr << "Error: Unknown argument " << arg << std::endl;
            return 1;
        }
    }

    ProcessorState state;
    size_t loadedWords = 0;
    if (!loadBinary(filename, state, loadedWords)) {
        return 1;
    }

    while (!state.halted) {
        if (state.instructionCount >= maxInstructions) {
            std::cerr << "Error: Instruction limit exceeded (" << maxInstructions
                      << " instructions executed, probable infinite loop)" << std::endl;
            return 1;
        }

        if (!executeStep(state, trace)) {
            return 1;
        }
    }

    // Stop cleanly on HALT, then print final registers and instruction count
    std::cout << "HALT executed. Execution finished.\n"
              << "Instructions executed: " << state.instructionCount << "\n"
              << "Final register state:\n"
              << "PC: 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << state.regPC
              << " (" << std::dec << state.regPC << ")\n"
              << "SP: 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << state.regSP
              << " (" << std::dec << state.regSP << ")\n"
              << "A:  0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << static_cast<uint32_t>(state.regA)
              << " (" << std::dec << state.regA << ")\n"
              << "B:  0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << static_cast<uint32_t>(state.regB)
              << " (" << std::dec << state.regB << ")\n";

    if (dump) {
        dumpMemory(state, loadedWords);
    }

    return 0;
}
