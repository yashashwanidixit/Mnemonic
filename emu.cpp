// Name: Yashashwani | ID: 2501AI49
// "I declare this is my own work."

#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <climits>
#include <iomanip>
#include <cstdio>

using namespace std;

// ============================================================================
// Constants and Configuration
// ============================================================================

// Processor memory: 65,536 32-bit words
constexpr size_t MEMORY_CAPACITY = 65536;

// Default limit on executed instructions to guard against infinite loops
constexpr uint64_t DEFAULT_MAX_INSTRUCTIONS = 1000000ULL;

// Opcode definitions matching IITPSx specification
constexpr unsigned OP_LDC    = 0;
constexpr unsigned OP_ADC    = 1;
constexpr unsigned OP_LDL    = 2;
constexpr unsigned OP_STL    = 3;
constexpr unsigned OP_LDNL   = 4;
constexpr unsigned OP_STNL   = 5;
constexpr unsigned OP_ADD    = 6;
constexpr unsigned OP_SUB    = 7;
constexpr unsigned OP_SHL    = 8;
constexpr unsigned OP_SHR    = 9;
constexpr unsigned OP_ADJ    = 10;
constexpr unsigned OP_A2SP   = 11;
constexpr unsigned OP_SP2A   = 12;
constexpr unsigned OP_CALL   = 13;
constexpr unsigned OP_RETURN = 14;
constexpr unsigned OP_BRZ    = 15;
constexpr unsigned OP_BRLZ   = 16;
constexpr unsigned OP_BR     = 17;
constexpr unsigned OP_HALT   = 18;
constexpr unsigned OP_MUL    = 19;
constexpr unsigned OP_DIV    = 20;

// ============================================================================
// Processor State Structure
// ============================================================================

struct ProcessorState {
    int regA = 0;
    int regB = 0;
    unsigned regPC = 0;
    unsigned regSP = (unsigned)(MEMORY_CAPACITY - 1); // Top of memory
    vector<int> memory;
    uint64_t instructionCount = 0;
    bool halted = false;

    ProcessorState() : memory(MEMORY_CAPACITY, 0) {}
};

// Returns mnemonic string for a given opcode
const char* getOpcodeName(unsigned op) {
    switch (op) {
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
bool opcodeHasOperand(unsigned op) {
    switch (op) {
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
bool loadBinary(const string& fname, ProcessorState& st, size_t& nWords) {
    ifstream file(fname, ios::binary | ios::ate);
    if (!file.is_open()) {
        fprintf(stderr, "Error: File not found: %s\n", fname.c_str());
        return false;
    }

    streamsize fileSize = file.tellg();
    if (fileSize < 0) {
        cerr << "Error: Could not read file size: " << fname << endl;
        return false;
    }

    if (fileSize % 4 != 0) {
        cerr << "Error: Bad file size (not a multiple of 4 bytes): " << fname
                  << " (" << fileSize << " bytes)" << endl;
        return false;
    }

    size_t words = (size_t)(fileSize / 4);
    if (words > MEMORY_CAPACITY) {
        cerr << "Error: Program exceeds memory capacity: " << words
                  << " words > " << MEMORY_CAPACITY << " words" << endl;
        return false;
    }

    file.seekg(0, ios::beg);
    vector<uint8_t> buffer((size_t)fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        cerr << "Error: Failed to read object file: " << fname << endl;
        return false;
    }

    for (size_t i = 0; i < words; ++i) {
        unsigned b0 = buffer[i * 4 + 0];
        unsigned b1 = buffer[i * 4 + 1];
        unsigned b2 = buffer[i * 4 + 2];
        unsigned b3 = buffer[i * 4 + 3];
        uint32_t word = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
        st.memory[i] = (int)word;
    }

    nWords = words;
    return true;
}

// ============================================================================
// Instruction Execution Engine
// ============================================================================

// Executes a single instruction cycle
bool executeStep(ProcessorState& st, bool trace) {
    if (st.regPC >= MEMORY_CAPACITY) {
        cerr << "Error: PC out of range (PC = " << st.regPC << ")" << endl;
        return false;
    }

    unsigned pc = st.regPC;
    int word = st.memory[pc];

    // Decode opcode (bottom 8 bits) and operand (upper 24 bits sign-extended)
    unsigned opcode = (unsigned)word & 0xFFu;
    int operand = word >> 8; // Arithmetic right shift performs 2's complement sign extension

    // PC is incremented BEFORE the instruction's action executes
    st.regPC++;

    if (trace) {
        cout << "PC: " << hex << setfill('0') << setw(8) << pc
                  << "  SP: " << setw(8) << st.regSP
                  << "  A: " << setw(8) << (unsigned)st.regA
                  << "  B: " << setw(8) << static_cast<unsigned>(st.regB)
                  << "  " << getOpcodeName(opcode);
        if (opcodeHasOperand(opcode)) {
            cout << " " << dec << operand << " (0x" << hex << (operand & 0xFFFFFF) << ")";
        }
        cout << dec << "\n";
    }

    switch (opcode) {
        case OP_LDC:
            st.regB = st.regA;
            st.regA = operand;
            break;

        case OP_ADC:
            st.regA = st.regA + operand;
            break;

        case OP_LDL: {
            int64_t addr = (int64_t)st.regSP + operand;
            if (addr < 0 || addr >= (int64_t)MEMORY_CAPACITY) {
                cerr << "Error: Memory access out of range (address " << addr
                          << " at PC " << pc << ")" << endl;
                return false;
            }
            st.regB = st.regA;
            st.regA = st.memory[(size_t)addr];
            break;
        }

        case OP_STL: {
            int64_t addr = static_cast<int64_t>(st.regSP) + operand;
            if (addr < 0 || addr >= static_cast<int64_t>(MEMORY_CAPACITY)) {
                cerr << "Error: Memory access out of range (address " << addr
                          << " at PC " << pc << ")" << endl;
                return false;
            }
            st.memory[static_cast<size_t>(addr)] = st.regA;
            st.regA = st.regB;
            break;
        }

        case OP_LDNL: {
            int64_t addr = (int64_t)st.regA + operand;
            if (addr < 0 || addr >= (int64_t)MEMORY_CAPACITY) {
                cerr << "Error: Memory access out of range (address " << addr
                          << " at PC " << pc << ")" << endl;
                return false;
            }
            st.regA = st.memory[(size_t)addr];
            break;
        }

        case OP_STNL: {
            int64_t addr = static_cast<int64_t>(st.regA) + operand;
            if (addr < 0 || addr >= static_cast<int64_t>(MEMORY_CAPACITY)) {
                cerr << "Error: Memory access out of range (address " << addr
                          << " at PC " << pc << ")" << endl;
                return false;
            }
            st.memory[static_cast<size_t>(addr)] = st.regB;
            break;
        }

        case OP_ADD:
            st.regA = st.regB + st.regA;
            break;

        case OP_SUB:
            st.regA = st.regB - st.regA;
            break;

        case OP_SHL: {
            unsigned sh = (unsigned)st.regA & 31u;
            st.regA = (int)((unsigned)st.regB << sh);
            break;
        }

        case OP_SHR: {
            unsigned shiftAmount = static_cast<unsigned>(st.regA) & 31u;
            st.regA = st.regB >> shiftAmount; // Arithmetic shift in C++ for signed type
            break;
        }

        case OP_ADJ:
            st.regSP = (unsigned)((int64_t)st.regSP + operand);
            break;

        case OP_A2SP:
            st.regSP = (unsigned)st.regA;
            st.regA = st.regB;
            break;

        case OP_SP2A:
            st.regB = st.regA;
            st.regA = (int)st.regSP;
            break;

        case OP_CALL:
            st.regB = st.regA;
            st.regA = (int)st.regPC;
            st.regPC = (unsigned)((int64_t)st.regPC + operand);
            break;

        case OP_RETURN:
            st.regPC = static_cast<unsigned>(st.regA);
            st.regA = st.regB;
            break;

        case OP_BRZ:
            if (st.regA == 0)
                st.regPC = (unsigned)((int64_t)st.regPC + operand);
            break;

        case OP_BRLZ:
            if (st.regA < 0) {
                st.regPC = static_cast<unsigned>(static_cast<int64_t>(st.regPC) + operand);
            }
            break;

        case OP_BR:
            st.regPC = static_cast<unsigned>(static_cast<int64_t>(st.regPC) + operand);
            break;

        case OP_HALT:
            st.halted = true;
            break;

        case OP_MUL:
            st.regA = st.regB * st.regA;
            break;

        case OP_DIV:
            if (st.regA == 0) {
                fprintf(stderr, "Error: Division by zero at PC %u\n", pc);
                return false;
            }
            if (st.regB == INT32_MIN && st.regA == -1) {
                // Guard against 32-bit signed division overflow
                st.regA = INT32_MIN;
            } else {
                st.regA = st.regB / st.regA;
            }
            break;

        default:
            cerr << "Error: Unknown opcode " << opcode << " (0x" << hex << opcode
                      << dec << ") at PC " << pc << endl;
            return false;
    }

    st.instructionCount++;
    return true;
}

// ============================================================================
// Memory Dump Function
// ============================================================================

// Dumps memory contents in hex format (4 words per line) up to highest modified/loaded address
void dumpMemory(const ProcessorState& st, size_t maxDumpWords) {
    cout << "\n========== Memory Dump ==========" << endl;
    size_t endWord = maxDumpWords;
    if (endWord > MEMORY_CAPACITY) endWord = MEMORY_CAPACITY;

    // Find highest non-zero address to avoid printing 64k lines of zeros
    size_t lastNonZero = 0;
    for (size_t i = 0; i < MEMORY_CAPACITY; ++i) {
        if (st.memory[i] != 0) {
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
        cout << uppercase << hex << setfill('0') << setw(8) << i << ": ";
        for (size_t j = 0; j < 4; ++j) {
            if (i + j < MEMORY_CAPACITY) {
                cout << setw(8) << (unsigned)st.memory[i + j] << " ";
            }
        }
        cout << "\n";
    }
    cout << dec;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <prog.o> [-trace] [-dump] [-max N]" << endl;
        return 1;
    }

    string fname = argv[1];
    bool trace = false;
    bool dump = false;
    uint64_t maxIns = DEFAULT_MAX_INSTRUCTIONS;

    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-trace") {
            trace = true;
        } else if (arg == "-dump") {
            dump = true;
        } else if (arg == "-max" && i + 1 < argc) {
            char* endPtr = nullptr;
            maxIns = strtoull(argv[++i], &endPtr, 0);
        } else {
            cerr << "Error: Unknown argument " << arg << endl;
            return 1;
        }
    }

    ProcessorState st;
    size_t nWords = 0;
    if (!loadBinary(fname, st, nWords)) {
        return 1;
    }

    while (!st.halted) {
        if (st.instructionCount >= maxIns) {
            cerr << "Error: Instruction limit exceeded (" << maxIns
                      << " instructions executed, probable infinite loop)" << endl;
            return 1;
        }

        if (!executeStep(st, trace)) {
            return 1;
        }
    }

    // Stop cleanly on HALT, then print final registers and instruction count
    cout << "HALT executed. Execution finished.\n"
              << "Instructions executed: " << st.instructionCount << "\n"
              << "Final register state:\n"
              << "PC: 0x" << uppercase << hex << setfill('0') << setw(8) << st.regPC
              << " (" << dec << st.regPC << ")\n"
              << "SP: 0x" << uppercase << hex << setfill('0') << setw(8) << st.regSP
              << " (" << dec << st.regSP << ")\n"
              << "A:  0x" << uppercase << hex << setfill('0') << setw(8) << (unsigned)st.regA
              << " (" << dec << st.regA << ")\n"
              << "B:  0x" << uppercase << hex << setfill('0') << setw(8) << static_cast<unsigned>(st.regB)
              << " (" << dec << st.regB << ")\n";

    if (dump) {
        dumpMemory(st, nWords);
    }

    return 0;
}
