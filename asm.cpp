// Name: Yashashwani | ID: 2501AI49
// "I declare this is my own work."

#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <cctype>
#include <cstdint>
#include <cerrno>
#include <algorithm>
#include <iomanip>
#include <cstdio>

using namespace std;

// ============================================================================
// Constants and Definitions
// ============================================================================

// 24-bit signed integer limits for instruction operands
constexpr int64_t INT24_MIN_VAL = -8388608L; // -2^23
constexpr int64_t INT24_MAX_VAL = 8388607L;  //  2^23 - 1
// constespr is used when we want the constant to get assigned at compile time
// instead of runtime itself
//  Bit manipulation masks
constexpr unsigned OPCODE_MASK = 0x000000FFu;
constexpr unsigned OPERAND24_MASK = 0x00FFFFFFu;
constexpr unsigned OPERAND_SHIFT = 8u;

// Special pseudo opcodes
// separately so that they dont clash with the actual opcode instructionsw
constexpr int OPCODE_DATA = -1;
constexpr int OPCODE_SET = -2;

// Operand requirement classification
enum class OperandType
{
    NONE,   // Instructions with no operand (e.g., add, sub, HALT, mul, div)
    VALUE,  // Instructions requiring a 24-bit value/offset (e.g., ldc, adc, ldl, stl, ldnl, stnl, adj)
    OFFSET, // Branch instructions (call, brz, brlz, br) where label operands become PC-relative offsets
    DATA,   // 'data' directive, stores full 32-bit word
    SET     // 'SET' pseudo-op, assigns value to label without emitting code
};

// Information structure for each instruction in the table
struct InstructionInfo
{
    const char *mnemonic;
    int opcode;
    OperandType opType;
};

// Instruction table: Single canonical table defining all processor instructions and pseudo-ops.
// Mnemonics are case sensitive and must match these entries exactly.
const InstructionInfo INSTRUCTION_TABLE[] = {
    {"ldc", 0, OperandType::VALUE},
    {"adc", 1, OperandType::VALUE},
    {"ldl", 2, OperandType::VALUE},
    {"stl", 3, OperandType::VALUE},
    {"ldnl", 4, OperandType::VALUE},
    {"stnl", 5, OperandType::VALUE},
    {"add", 6, OperandType::NONE},
    {"sub", 7, OperandType::NONE},
    {"shl", 8, OperandType::NONE},
    {"shr", 9, OperandType::NONE},
    {"adj", 10, OperandType::VALUE},
    {"a2sp", 11, OperandType::NONE},
    {"sp2a", 12, OperandType::NONE},
    {"call", 13, OperandType::OFFSET},
    {"return", 14, OperandType::NONE},
    {"brz", 15, OperandType::OFFSET},
    {"brlz", 16, OperandType::OFFSET},
    {"br", 17, OperandType::OFFSET},
    {"HALT", 18, OperandType::NONE},
    {"mul", 19, OperandType::NONE},
    {"div", 20, OperandType::NONE},
    {"data", OPCODE_DATA, OperandType::DATA},
    {"SET", OPCODE_SET, OperandType::SET}};

constexpr size_t INSTRUCTION_TABLE_SIZE = sizeof(INSTRUCTION_TABLE) / sizeof(INSTRUCTION_TABLE[0]);
// size_t for non negative values
//  ============================================================================
//  Data Structures for Assembly Pipeline
//  ============================================================================

// Symbol table entry holding label properties
struct SymbolEntry
{
    int value = 0;   // Address or SET value
    int definedLine = 0; // 1-based source line where defined
    bool isUsed = false; // Track if referenced by any instruction
    bool isSet = false;  // True if defined via SET pseudo-op
};

// Diagnostic message (error or warning)
struct Diagnostic
{
    int line;
    bool isWarning;
    string message;
};

// Representation of a tokenized line of source assembly
struct ParsedLine
{
    int lineNumber = 0;
    string rawLine;
    vector<string> labels;
    bool hasStatement = false;
    string mnem;
    bool hasOperand = false;
    string op;
};

// Represents one emitted 32-bit machine word in memory
struct AssembledWord
{
    unsigned address;
    uint32_t machineCode;//stores the 32 bit bit pattern
    string mnemonic;
    string operandStr;
    //because an operand can be a stirng or a number - br loop
    vector<string> labelsAtAddress;//labels defined at this line 
    bool isBranch;
    string targetLabel;//label used at this line /branchs destination, used as an operand 
    //like entry: alias: ldc 42
//ldc entry
//ldc alias
//HALT - here ldc is target label for line 1 
    int rawOffset;
    //branch offset
    // Error marker for lines that failed to assemble. When isError is true the
    // address slot is still reserved (pc advances past it) but the listing prints
    // errorText instead of a machine word: any 32-bit value, including 0, would
    // be indistinguishable from a real instruction such as `ldc 0`, so no fake
    // word is ever emitted. Object-file writers must skip such rows.
    bool isError = false;
    string errorText;
    //for unknown mnemonic insconsistency
};

// Passes identifier
enum class Pass
{
    PASS_1,
    PASS_2
};

// ============================================================================
// Utility and Helper Functions
// ============================================================================

// Trims leading and trailing whitespace from a string
string trim(const string &str)
{
    const char *whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == string::npos)
        return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// Strips comments starting with ';' from the source line
string stripComment(const string &line)
{
    size_t pos = line.find(';');
    if (pos != string::npos)
        return line.substr(0, pos);
    return line;
}

// Validates label name: starts with an alphabetic letter, followed by alphanumeric characters
bool isValidLabelName(const string &name)
{//we have used unsigned because some chars might have a negative code
    if (name.empty())
        return false;
    if (!isalpha((unsigned char)name[0]))
        return false;
    for (size_t i = 1; i < name.size(); ++i)
    {
        if (!isalnum(static_cast<unsigned char>(name[i])))
            return false;
    }
    return true;
}

// Searches the instruction table with exact, case-sensitive comparison
const InstructionInfo *findInstruction(const string &mnem)
{
    // Deliberately exact comparison: mnemonics are case sensitive.
    for (size_t i = 0; i < INSTRUCTION_TABLE_SIZE; i++)
    {
        if (mnem == INSTRUCTION_TABLE[i].mnemonic)
        {
            return &INSTRUCTION_TABLE[i];
        }
    }
    return nullptr;
}


//converts string to number 
bool parseNumber(const string &tok, int64_t &outVal)
{
    if (tok.empty())
        return false;
    char *endPtr = nullptr;
    errno = 0;
    //endptr- function sets this to the first number after the valid parse number 
    long val = strtol(tok.c_str(), &endPtr, 0);
    //didtn use c++ alternatives coz it was mentioened in the pdf so that we can get to know 
    //the type(octal,hex etc ) , the first invalid char and 
    //0 means it automatically detects the base 
    if (errno != 0)
        return false;
        //first condition satisfied when the first char is not a number for eg 
    if (endPtr == tok.c_str() || *endPtr != '\0')
        return false;
    outVal = (int64_t)val;
    return true;
}

// convert xyz.asm to xyz 
string getBaseFilename(const string &filepath)
{
    size_t dotPos = filepath.find_last_of('.');
    size_t slashPos = filepath.find_last_of("/\\");
    if (dotPos != string::npos && (slashPos == string::npos || dotPos > slashPos))
    {
        return filepath.substr(0, dotPos);
    }
    return filepath;
}

// Checks if token appears to be intended as a numeric literal (starts with digit or sign)
bool isNumericPrefix(const string &tok)
{
    if (tok.empty())
        return false;
    if (isdigit((unsigned char)tok[0]))
        return true;
    if (tok[0] == '+' || tok[0] == '-')
    {
        return true;
    }
    return false;
}

// ============================================================================
// Source Line Parsing
// ============================================================================

// Parses a single raw source line into labels, mnemonic, operand
ParsedLine parseLine(const string &rawLine, int ln, vector<Diagnostic> &diags)
{
    ParsedLine pl;
    pl.lineNumber = ln;
    pl.rawLine = rawLine;

    string cleaned = stripComment(rawLine);//removes the text after ;
    cleaned = trim(cleaned);//trims the leading and trailing whitespaces 
    if (cleaned.empty())
    {
        return pl;
    }

    // Process all labels defined on this line (preceding colons)
    while (true)
    {
        size_t colonPos = cleaned.find(':');
        if (colonPos == string::npos)
        {
            break;
        }

        string labelPart = trim(cleaned.substr(0, colonPos));
        if (labelPart.empty() || !isValidLabelName(labelPart))
        {
            diags.push_back({ln, false, "bogus label name '" + labelPart + "'"});
        }
        else
        {
            pl.labels.push_back(labelPart);
        }

        cleaned = trim(cleaned.substr(colonPos + 1));
        if (cleaned.empty())
        {
            break;
        }
        //repeat until all labels are included like start: loop: ldc 5
    }

    if (cleaned.empty())
    {
        return pl;
    }
    //after this loop the words before the : would be stored as a label and cleaned is set to whatever follows the label
    // so for unused label cleaned will now be empty

    // Extract mnemonic (cleaned is already trimmed, so no leading whitespace to skip)
    size_t i = 0;
//size_t covers till 2^64 -1 and unsinged till 2^32-1

//The main reason is that i is a string position: cleaned.size() returns string::size_type
    size_t mStart = i;
    while (i < cleaned.size() && cleaned[i] != ' ' && cleaned[i] != '\t' && cleaned[i] != ',')
    {
        i++;
    }
    //string.substr(pos, count);
    //count is optional take it to the end if not present 
    pl.mnem = cleaned.substr(mStart, i - mStart);
    pl.hasStatement = true;

    // Skip whitespace between mnemonic and operand
    while (i < cleaned.size() && (cleaned[i] == ' ' || cleaned[i] == '\t'))
    {
        i++;
    }

    if (i >= cleaned.size())
    {
        return pl; // No operand
    }

    // Extract operand token: characters up to next whitespace or comma
    size_t opSt = i;
    while (i < cleaned.size() && cleaned[i] != ' ' && cleaned[i] != '\t' && cleaned[i] != ',')
    {
        i++;
    }

    pl.hasOperand = true;
    pl.op = cleaned.substr(opSt, i - opSt);

    return pl;
}

// ============================================================================
// Core Assembly Engine (Two-Pass Unified Routine)
// ============================================================================

// Assembles all lines in either Pass 1 or Pass 2.
// Single routine parameterized by 'pass' to ensure complete architectural consistency.
bool assemblePass(
    const vector<ParsedLine> &lines,
    Pass pass,
    map<string, SymbolEntry> &symbolTable,
    vector<AssembledWord> &words,
    vector<Diagnostic> &diagnostics)
{
    unsigned pc = 0;
    bool hasErrors = false;

    for (const ParsedLine &pl : lines)
    {
        int lineNum = pl.lineNumber;
        // First error on this source line found during the current pass, if any.
        // Pass 1 reports it to diagnostics; Pass 2 records it here so the listing
        // can print an ERROR row for the line instead of a misleading zero word.
        string lineError;

        // Pass 1: Define labels
        if (pass == Pass::PASS_1)
        {
            for (const string &lbl : pl.labels)
            //pl.labels only contains the labels that are defined on that line 
            //not the ones that are used in the line 
            {
                if (symbolTable.find(lbl) != symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "duplicate label '" + lbl + "'"});
                    hasErrors = true;
                }
                else
                {
                    SymbolEntry entry;
                    entry.value = (int)pc;//value is int because every label holds address but SET holds 32 bit constant 
                    entry.definedLine = lineNum;
                    entry.isUsed = false;
                    entry.isSet = false;
                    symbolTable[lbl] = entry;
                }
            }
        }

        if (!pl.hasStatement)
        {
            continue;
        }

        // Validate mnemonic
        const InstructionInfo *inst = findInstruction(pl.mnem);
        if (!inst)
        {
            hasErrors = true;
            if (pass == Pass::PASS_1)
            {
                diagnostics.push_back({lineNum, false, "unknown mnemonic '" + pl.mnem + "'"});
                // Reserve the address slot so Pass 1 addresses match the Pass 2
                // listing exactly, even for lines that fail to assemble.
                pc++;
                continue;
            }
            // PASS_2: the address slot is reserved (pc advances below) but no
            // machine word is invented for an unrecognized instruction. The
            // listing shows the diagnostic text instead (see writeListingFile).
            AssembledWord aw;
            aw.address = pc;
            aw.machineCode = 0; // unused when iserror is true; never written out
            aw.mnemonic = pl.mnem;//is *inst rh
            aw.operandStr = pl.hasOperand ? pl.op : "";
            aw.labelsAtAddress = pl.labels;
            aw.isBranch = false;
            aw.targetLabel = "";
            aw.rawOffset = 0;
            aw.isError = true;
            aw.errorText = "unknown mnemonic '" + pl.mnem + "'";
            words.push_back(aw);
            pc++;
            continue;
        }


        // Validate operand presence
        if (inst->opType == OperandType::NONE)
        {
            if (pl.hasOperand)
            {
                if (pass == Pass::PASS_1)
                {
                    diagnostics.push_back({lineNum, false, "unexpected operand for '" + pl.mnem + "'"});
                }
                else if (lineError.empty())
                {
                    lineError = "unexpected operand for '" + pl.mnem + "'";
                }
                hasErrors = true;
            }
        }
        else
        {
            if (!pl.hasOperand)
            {
                if (pass == Pass::PASS_1)
                {
                    diagnostics.push_back({lineNum, false, "missing operand for '" + pl.mnem + "'"});
                }
                else if (lineError.empty())
                {
                    lineError = "missing operand for '" + pl.mnem + "'";
                }
                hasErrors = true;
            }
        }

        // Validate operand syntax: Pass 1 reports the diagnostic, Pass 2 records
        // the same text so the listing row can mirror it (no duplicate report).
        if (pl.hasOperand && inst->opType != OperandType::NONE)
        {
            if (isNumericPrefix(pl.op))
            {
                int64_t dummy = 0;
                if (!parseNumber(pl.op, dummy))
                {
                    hasErrors = true;
                    if (pass == Pass::PASS_1)
                    {
                        diagnostics.push_back({lineNum, false, "invalid number '" + pl.op + "'"});
                    }
                    else if (lineError.empty())
                    {
                        lineError = "invalid number '" + pl.op + "'";
                    }
                }
            }
            else
            {
                if (!isValidLabelName(pl.op))
                {
                    hasErrors = true;
                    if (pass == Pass::PASS_1)
                    {
                        diagnostics.push_back({lineNum, false, "bogus label name '" + pl.op + "'"});
                    }
                    else if (lineError.empty())
                    {
                        lineError = "bogus label name '" + pl.op + "'";
                    }
                }
            }
        }

        // Handle SET pseudo-operation
        if (inst->opType == OperandType::SET)
        {
            if (pl.labels.empty())
            {
                if (pass == Pass::PASS_1)
                {
                    diagnostics.push_back({lineNum, false, "SET directive requires a label on the same line"});
                    hasErrors = true;
                }
            }
            if (pl.hasOperand)
            {
                int64_t numVal = 0;
                if (parseNumber(pl.op, numVal))
                {
                    if (pass == Pass::PASS_1)
                    {
                        for (const string &lbl : pl.labels)
                        {
                            symbolTable[lbl].value = (int)numVal;
                            symbolTable[lbl].isSet = true;
                        }
                    }
                }
                else
                {
                    // Operand is a label
                    if (pass == Pass::PASS_2)
                    {
                        auto it = symbolTable.find(pl.op);
                        if (it == symbolTable.end())
                        {
                            diagnostics.push_back({lineNum, false, "no such label '" + pl.op + "'"});
                            hasErrors = true;
                        }
                        else
                        {
                            it->second.isUsed = true;
                            for (const string &lbl : pl.labels)
                            {
                                symbolTable[lbl].value = it->second.value;
                                symbolTable[lbl].isSet = true;
                            }
                        }
                    }
                }
            }
            // SET does NOT advance PC
            continue;
        }

        // In Pass 1, we advance PC for instructions/data and move on
        if (pass == Pass::PASS_1)
        {
            pc++;
            continue;
        }

        // ====================================================================
        // Pass 2 Code Generation & Label Resolution
        // ====================================================================
        // Remember how many diagnostics exist before generating this line: any
        // Pass-2 diagnostic raised below (no such label, operand out of range)
        // turns this row into an ERROR marker instead of a fake machine word.
        size_t diagBefore = diagnostics.size();
        uint32_t word = 0;
        bool isBranch = (inst->opType == OperandType::OFFSET);
        string tgt;
        int rawOffset = 0;

        if (inst->opType == OperandType::NONE)
        {
            word = (unsigned)(inst->opcode & 0xFF);
        }
        else if (inst->opType == OperandType::DATA)
        {
            int64_t numVal = 0;
            if (parseNumber(pl.op, numVal))
            {
                word = (unsigned)numVal;
            }
            else if (isValidLabelName(pl.op))
            {
                auto it = symbolTable.find(pl.op);
                if (it == symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "no such label '" + pl.op + "'"});
                    hasErrors = true;
                }
                else
                {
                    it->second.isUsed = true;
                    word = (unsigned)it->second.value;
                }
            }
        }
        else if (inst->opType == OperandType::VALUE)
        {
            int64_t numVal = 0;
            int val = 0;
            if (parseNumber(pl.op, numVal))
            {
                if (numVal < INT24_MIN_VAL || numVal > INT24_MAX_VAL)
                {
                    diagnostics.push_back({lineNum, false, "operand out of range (" + pl.op + ")"});
                    hasErrors = true;
                }
                val = static_cast<int>(numVal);
            }
            else if (isValidLabelName(pl.op))
            {
                auto it = symbolTable.find(pl.op);
                if (it == symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "no such label '" + pl.op + "'"});
                    hasErrors = true;
                }
                else
                {
                    it->second.isUsed = true;
                    val = it->second.value;
                    if (val < INT24_MIN_VAL || val > INT24_MAX_VAL)
                    {
                        diagnostics.push_back({lineNum, false, "operand out of range (" + pl.op + ")"});
                        hasErrors = true;
                    }
                }
            }
            word = (((unsigned)val & 0xFFFFFF) << 8) |
                   ((unsigned)inst->opcode & 0xFF);
        }
        else if (inst->opType == OperandType::OFFSET)
        {
            int64_t numVal = 0;
            int offset = 0;
            if (parseNumber(pl.op, numVal))
            {
                // Numeric operand for branch is used as raw offset directly
                if (numVal < INT24_MIN_VAL || numVal > INT24_MAX_VAL)
                {
                    diagnostics.push_back({lineNum, false, "operand out of range (" + pl.op + ")"});
                    hasErrors = true;
                }
                offset = (int)numVal;
                rawOffset = offset;
            }
            else if (isValidLabelName(pl.op))
            {
                auto it = symbolTable.find(pl.op);
                if (it == symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "no such label '" + pl.op + "'"});
                    hasErrors = true;
                }
                else
                {
                    it->second.isUsed = true;
                    tgt = pl.op;
                    int targetAddr = it->second.value;
                    offset = targetAddr - (int)(pc + 1);
                    rawOffset = offset;
                    if (offset < INT24_MIN_VAL || offset > INT24_MAX_VAL)
                    {
                        diagnostics.push_back({lineNum, false, "operand out of range (" + pl.op + ")"});
                        hasErrors = true;
                    }
                }
            }
            word = (((unsigned)offset & 0xFFFFFF) << 8) |
                   (static_cast<unsigned>(inst->opcode) & 0xFF);
        }

        // A fresh Pass-2 diagnostic for this line (e.g. no such label, operand
        // out of range) means the computed word is garbage: keep the address
        // slot but show the message instead. Parse-time label-name problems
        // (e.g. a bogus `0def:` prefix) do not mark the row, because the
        // instruction itself still assembles to a valid word.
        if (diagnostics.size() > diagBefore && lineError.empty())
        {
            lineError = diagnostics[diagBefore].message;
        }

        AssembledWord aw;
        aw.address = pc;
        if (!lineError.empty())
        {
            aw.machineCode = 0; // unused when isError is true; never written out
            aw.mnemonic = pl.mnem;
            aw.operandStr = pl.hasOperand ? pl.op : "";
            aw.labelsAtAddress = pl.labels;
            aw.isBranch = false;
            aw.targetLabel = "";
            aw.rawOffset = 0;
            aw.isError = true;
            aw.errorText = lineError;
        }
        else
        {
            aw.machineCode = word;
            aw.mnemonic = pl.mnem;
            aw.operandStr = pl.op;
            aw.labelsAtAddress = pl.labels;
            aw.isBranch = isBranch;
            aw.targetLabel = tgt;
            aw.rawOffset = rawOffset;
            aw.isError = false;
            aw.errorText = "";
        }
        words.push_back(aw);

        pc++;
    }

    // In Pass 2, check for unused labels and issue warnings
    if (pass == Pass::PASS_2)
    {
        for (const auto &pair : symbolTable)
        {
            if (!pair.second.isUsed)
            {
                diagnostics.push_back({pair.second.definedLine, true, "unused label '" + pair.first + "'"});
            }
        }
    }

    return !hasErrors;
}

// ============================================================================
// Output Writers (Object file and Advanced Listing file)
// ============================================================================

// Writes raw binary object file (32-bit little-endian words, address 0 first, no header)
bool writeObjectFile(const string &filename, const vector<AssembledWord> &words)
{
    ofstream out(filename, ios::binary);
    if (!out.is_open())
    {
        fprintf(stderr, "Error: Could not open output object file %s\n", filename.c_str());
        return false;
    }

    for (const AssembledWord &aw : words)
    {
        // Error-marker rows hold no machine code (their machineCode field is a
        // dummy zero, which would read back as a real `ldc 0`), so they are
        // never written to the object file.
        if (aw.isError)
        {
            continue;
        }
        uint32_t code = aw.machineCode;
        uint8_t bytes[4];
        bytes[0] = (uint8_t)(code & 0xFF);
        bytes[1] = static_cast<uint8_t>((code >> 8) & 0xFF);
        bytes[2] = (uint8_t)((code >> 16) & 0xFF);
        bytes[3] = static_cast<uint8_t>((code >> 24) & 0xFF);
        out.write(reinterpret_cast<const char *>(bytes), 4);
    }

    return true;
}

// Writes listing file in the preferred advanced format
bool writeListingFile(
    const string &filename,
    const vector<AssembledWord> &words,
    const map<string, SymbolEntry> &syms)
{
    ofstream out(filename);
    if (!out.is_open())
    {
        cerr << "Error: Could not open output listing file " << filename << endl;
        return false;
    }

    // Build reverse map of address -> labels for branch label resolution
    map<int, string> addrToLabel;
    for (const auto &pair : syms)
    {
        if (!pair.second.isSet)
        {
            // Prefer the first or shortest label
            if (addrToLabel.find(pair.second.value) == addrToLabel.end())
            {
                addrToLabel[pair.second.value] = pair.first;
            }
        }
    }

    for (const AssembledWord &aw : words)
    {
        // Print label line if any labels defined at this address
        for (const string &lbl : aw.labelsAtAddress)
        {
            out << uppercase << hex << setfill('0') << setw(8) << aw.address
                << " " << lbl << ":" << endl;
        }

        // Failed lines keep their address slot but print the diagnostic instead
        // of a machine word: any 32-bit value, including 0, would be
        // indistinguishable from a real instruction such as `ldc 0`.
        // Format: AAAAAAAA ERROR <message>
        if (aw.isError)
        {
            out << uppercase << hex << setfill('0') << setw(8) << aw.address
                << " ERROR " << aw.errorText << "\n";
            continue;
        }

        // Instruction word line: AAAAAAAA XXXXXXXX mnemonic operand
        out << uppercase << hex << setfill('0') << setw(8) << aw.address
            << " " << setw(8) << aw.machineCode
            << " " << aw.mnemonic;

        if (!aw.operandStr.empty())
        {
            if (aw.isBranch)
            {
                int targetAddr = (int)(aw.address + 1) + aw.rawOffset;
                auto it = addrToLabel.find(targetAddr);
                if (it != addrToLabel.end())
                {
                    out << " " << it->second;
                }
                else if (!aw.targetLabel.empty())
                {
                    out << " " << aw.targetLabel;
                }
                else
                {
                    out << " " << aw.operandStr;
                }
            }
            else
            {
                out << " " << aw.operandStr;
            }
        }
        out << endl;
    }

    return true;
}


//argc =  the umber of words in the command line and *argv (vector of those words)
int main(int argc, char *argv[])
{//cerr is unbuffered , flushes the output buffer immedaitely after insertion , so it 
    //appears on the string right away even if the program crashes 
    if (argc < 2)
    {//used argv[0] instead of hardcoding asm so that it can be accessed from someother file path too 
        cerr << "Usage: " << argv[0] << " <source.asm>" << endl;
        return 1;
    }

    string inputPath = argv[1];
    //ifstream- a type that represnets file opened for reading 
    ifstream inFile(inputPath);
    if (!inFile.is_open())
    {
        fprintf(stderr, "Error: Could not open input file %s\n", inputPath.c_str());
        return 1;
    }
//we are creaitng a separate data structure for the error message and not just combining into the 
//parsed line strcture because we might have multiple errors in a single error

    vector<Diagnostic> diagnostics;
    vector<ParsedLine> parsedLines;
    string lineStr;
    int lineCounter = 1;

    while (getline(inFile, lineStr))
    {
        ParsedLine pl = parseLine(lineStr, lineCounter, diagnostics);
        parsedLines.push_back(pl);
        lineCounter++;
    }
    //std ifstream does this automatically so isnt really required 
    inFile.close();
    //matters more when more than 1 programs are open doesnt really matter here

    map<string, SymbolEntry> symbolTable;
    vector<AssembledWord> dummyWords;

    // Pass 1: Build symbol table and check basic syntax
    assemblePass(parsedLines, Pass::PASS_1, symbolTable, dummyWords, diagnostics);

    // Pass 2: Code generation and label resolution
    vector<AssembledWord> codeWords;
    assemblePass(parsedLines, Pass::PASS_2, symbolTable, codeWords, diagnostics);

    // Sort diagnostics by line number
    sort(diagnostics.begin(), diagnostics.end(), [](const Diagnostic &a, const Diagnostic &b)
              {
                  if (a.line != b.line)
                      return a.line < b.line;
                  return a.isWarning < b.isWarning; // errors before warnings
              });

    // Check if any errors occurred
    bool hasErrors = false;
    for (const Diagnostic &diag : diagnostics)
    {
        if (diag.isWarning)
        {
            cerr << "Warning: Line " << diag.line << ": " << diag.message << endl;
        }
        else
        {
            cerr << "Error: Line " << diag.line << ": " << diag.message << "\n";
            hasErrors = true;
        }
    }

    string baseName = getBaseFilename(inputPath);
    string objFilename = baseName + ".o";
    string lstFilename = baseName + ".lst";

    // Write listing file
    writeListingFile(lstFilename, codeWords, symbolTable);

    // Produce no object file if any error occurred
    if (hasErrors)
    {
        cerr << "Assembly failed with errors. No object file generated." << endl;
        return 1;
    }

    // Write binary object file
    if (!writeObjectFile(objFilename, codeWords))
    {
        return 1;
    }

    return 0;
}
