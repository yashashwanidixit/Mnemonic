

#include <iostream>
#include <fstream>

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <iomanip>

// ============================================================================
// Constants and Definitions
// ============================================================================

// 24-bit signed integer limits for instruction operands
constexpr int64_t INT24_MIN_VAL = -8388608L; // -2^23
constexpr int64_t INT24_MAX_VAL = 8388607L;  //  2^23 - 1
// constespr is used when we want the constant to get assigned at compile time
// instead of runtime itself
//  Bit manipulation masks
constexpr uint32_t OPCODE_MASK = 0x000000FFu;
constexpr uint32_t OPERAND24_MASK = 0x00FFFFFFu;
constexpr unsigned int OPERAND_SHIFT = 8u;

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
    int32_t value = 0;   // Address or SET value
    int definedLine = 0; // 1-based source line where defined
    bool isUsed = false; // Track if referenced by any instruction
    bool isSet = false;  // True if defined via SET pseudo-op
};

// Diagnostic message (error or warning)
struct Diagnostic
{
    int line;
    bool isWarning;
    std::string message;
};

// Representation of a tokenized line of source assembly
struct ParsedLine
{
    int lineNumber = 0;
    std::string rawLine;
    std::vector<std::string> labels;
    bool hasStatement = false;
    std::string mnemonic;
    bool hasOperand = false;
    std::string operandToken;
    bool extraTextError = false;
    std::string extraText;
};

// Represents one emitted 32-bit machine word in memory
struct AssembledWord
{
    uint32_t address;
    uint32_t machineCode;
    std::string mnemonic;
    std::string operandStr;
    std::vector<std::string> labelsAtAddress;
    bool isBranch;
    std::string targetLabel;
    int32_t rawOffset;
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
static std::string trim(const std::string &str)
{
    const char *whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// Strips comments starting with ';' from the source line
static std::string stripComment(const std::string &line)
{
    size_t pos = line.find(';');
    if (pos != std::string::npos)
    {
        return line.substr(0, pos);
    }
    return line;
}

// Validates label name: starts with an alphabetic letter, followed by alphanumeric characters
static bool isValidLabelName(const std::string &name)
{//we have used unsigned because some chars might have a negative code
    if (name.empty())
        return false;
    if (!std::isalpha(static_cast<unsigned char>(name[0])))
        return false;
    for (size_t i = 1; i < name.size(); ++i)
    {
        if (!std::isalnum(static_cast<unsigned char>(name[i])))
            return false;
    }
    return true;
}

// Searches the instruction table with exact, case-sensitive comparison
static const InstructionInfo *findInstruction(const std::string &mnemonic)
{
    // Deliberately exact comparison: mnemonics are case sensitive.
    for (size_t i = 0; i < INSTRUCTION_TABLE_SIZE; ++i)
    {
        if (mnemonic == INSTRUCTION_TABLE[i].mnemonic)
        {
            return &INSTRUCTION_TABLE[i];
        }
    }
    return nullptr;
}


//converts string to number 
static bool parseNumber(const std::string &token, int64_t &outVal)
{
    if (token.empty())
        return false;
    char *endPtr = nullptr;
    errno = 0;
    //endptr- function sets this to the first number after the valid parse number 
    long val = std::strtol(token.c_str(), &endPtr, 0);
    //didtn use c++ alternatives coz it was mentioened in the pdf so that we can get to know 
    //the type(octal,hex etc ) , the first invalid char and 
    //0 means it automatically detects the base 
    if (errno != 0)
        return false;
        //first condition satisfied when the first char is not a number for eg 
    if (endPtr == token.c_str() || *endPtr != '\0')
        return false;
    outVal = val;
    return true;
}

// convert xyz.asm to xyz 
static std::string getBaseFilename(const std::string &filepath)
{
    size_t dotPos = filepath.find_last_of('.');
    size_t slashPos = filepath.find_last_of("/\\");
    if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos))
    {
        return filepath.substr(0, dotPos);
    }
    return filepath;
}

// Checks if token appears to be intended as a numeric literal (starts with digit or sign)
static bool isNumericPrefix(const std::string &token)
{
    if (token.empty())
        return false;
    if (std::isdigit(static_cast<unsigned char>(token[0])))
        return true;
    if (token[0] == '+' || token[0] == '-')
    {
        return true;
    }
    return false;
}

// ============================================================================
// Source Line Parsing
// ============================================================================

// Parses a single raw source line into labels, mnemonic, operand, and extra text
static ParsedLine parseLine(const std::string &rawLine, int lineNumber, std::vector<Diagnostic> &diagnostics)
{
    ParsedLine pl;
    pl.lineNumber = lineNumber;
    pl.rawLine = rawLine;

    std::string cleaned = stripComment(rawLine);//removes the text after ;
    cleaned = trim(cleaned);//trims the leading and trailing whitespaces 
    if (cleaned.empty())
    {
        return pl;
    }

    // Process all labels defined on this line (preceding colons)
    while (true)
    {
        size_t colonPos = cleaned.find(':');
        if (colonPos == std::string::npos)
        {
            break;
        }

        std::string labelPart = trim(cleaned.substr(0, colonPos));
        if (labelPart.empty() || !isValidLabelName(labelPart))
        {
            diagnostics.push_back({lineNumber, false, "bogus label name '" + labelPart + "'"});
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

    // Extract mnemonic
    size_t idx = 0;
    while (idx < cleaned.size() && (cleaned[idx] == ' ' || cleaned[idx] == '\t'))
    {
        idx++;
    }
    if (idx >= cleaned.size())
    {
        return pl;
    }
//size_t covers till 2^64 -1 and unsinged till 2^32-1

//The main reason is that idx is a string position: cleaned.size() returns std::string::size_type
    size_t mnemStart = idx;
    while (idx < cleaned.size() && cleaned[idx] != ' ' && cleaned[idx] != '\t' && cleaned[idx] != ',')
    {
        idx++;
    }
    //string.substr(pos, count);
    //count is optional take it to the end if not present 
    pl.mnemonic = cleaned.substr(mnemStart, idx - mnemStart);
    pl.hasStatement = true;

    // Skip whitespace between mnemonic and operand
    while (idx < cleaned.size() && (cleaned[idx] == ' ' || cleaned[idx] == '\t'))
    {
        idx++;
    }

    if (idx >= cleaned.size())
    {
        return pl; // No operand
    }

    // Extract operand token: characters up to next whitespace or comma
    size_t opStart = idx;
    while (idx < cleaned.size() && cleaned[idx] != ' ' && cleaned[idx] != '\t' && cleaned[idx] != ',')
    {
        idx++;
    }

    pl.hasOperand = true;
    pl.operandToken = cleaned.substr(opStart, idx - opStart);

    // Any remaining text is extra text
    std::string extra = trim(cleaned.substr(idx));
    if (!extra.empty())
    {
        pl.extraTextError = true;
        pl.extraText = extra;
    }

    return pl;
}

// ============================================================================
// Core Assembly Engine (Two-Pass Unified Routine)
// ============================================================================

// Assembles all lines in either Pass 1 or Pass 2.
// Single routine parameterized by 'pass' to ensure complete architectural consistency.
static bool assemblePass(
    const std::vector<ParsedLine> &lines,
    Pass pass,
    std::map<std::string, SymbolEntry> &symbolTable,
    std::vector<AssembledWord> &assembledWords,
    std::vector<Diagnostic> &diagnostics)
{
    uint32_t pc = 0;
    bool hasErrors = false;

    for (const ParsedLine &pl : lines)
    {
        int lineNum = pl.lineNumber;

        // Pass 1: Define labels
        if (pass == Pass::PASS_1)
        {
            for (const std::string &lbl : pl.labels)
            {
                if (symbolTable.find(lbl) != symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "duplicate label '" + lbl + "'"});
                    hasErrors = true;
                }
                else
                {
                    SymbolEntry entry;
                    entry.value = static_cast<int32_t>(pc);
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
        const InstructionInfo *inst = findInstruction(pl.mnemonic);
        if (!inst)
        {
            if (pass == Pass::PASS_1)
            {
                diagnostics.push_back({lineNum, false, "unknown mnemonic '" + pl.mnemonic + "'"});
            }
            hasErrors = true;
            continue;
        }

        // Validate operand presence
        if (inst->opType == OperandType::NONE)
        {
            if (pl.hasOperand)
            {
                if (pass == Pass::PASS_1)
                {
                    diagnostics.push_back({lineNum, false, "unexpected operand for '" + pl.mnemonic + "'"});
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
                    diagnostics.push_back({lineNum, false, "missing operand for '" + pl.mnemonic + "'"});
                }
                hasErrors = true;
            }
        }

        // Validate extra text at end of line
        if (pl.extraTextError)
        {
            if (pass == Pass::PASS_1)
            {
                diagnostics.push_back({lineNum, false, "extra text at end of line: '" + pl.extraText + "'"});
            }
            hasErrors = true;
        }

        // Validate operand syntax in Pass 1
        if (pass == Pass::PASS_1 && pl.hasOperand && inst->opType != OperandType::NONE)
        {
            if (isNumericPrefix(pl.operandToken))
            {
                int64_t dummy = 0;
                if (!parseNumber(pl.operandToken, dummy))
                {
                    diagnostics.push_back({lineNum, false, "invalid number '" + pl.operandToken + "'"});
                    hasErrors = true;
                }
            }
            else
            {
                if (!isValidLabelName(pl.operandToken))
                {
                    diagnostics.push_back({lineNum, false, "bogus label name '" + pl.operandToken + "'"});
                    hasErrors = true;
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
                if (parseNumber(pl.operandToken, numVal))
                {
                    if (pass == Pass::PASS_1)
                    {
                        for (const std::string &lbl : pl.labels)
                        {
                            symbolTable[lbl].value = static_cast<int32_t>(numVal);
                            symbolTable[lbl].isSet = true;
                        }
                    }
                }
                else
                {
                    // Operand is a label
                    if (pass == Pass::PASS_2)
                    {
                        auto it = symbolTable.find(pl.operandToken);
                        if (it == symbolTable.end())
                        {
                            diagnostics.push_back({lineNum, false, "no such label '" + pl.operandToken + "'"});
                            hasErrors = true;
                        }
                        else
                        {
                            it->second.isUsed = true;
                            for (const std::string &lbl : pl.labels)
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
        uint32_t word = 0;
        bool isBranch = (inst->opType == OperandType::OFFSET);
        std::string targetLabel;
        int32_t rawOffset = 0;

        if (inst->opType == OperandType::NONE)
        {
            word = static_cast<uint32_t>(inst->opcode & OPCODE_MASK);
        }
        else if (inst->opType == OperandType::DATA)
        {
            int64_t numVal = 0;
            if (parseNumber(pl.operandToken, numVal))
            {
                word = static_cast<uint32_t>(numVal);
            }
            else if (isValidLabelName(pl.operandToken))
            {
                auto it = symbolTable.find(pl.operandToken);
                if (it == symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "no such label '" + pl.operandToken + "'"});
                    hasErrors = true;
                }
                else
                {
                    it->second.isUsed = true;
                    word = static_cast<uint32_t>(it->second.value);
                }
            }
        }
        else if (inst->opType == OperandType::VALUE)
        {
            int64_t numVal = 0;
            int32_t val = 0;
            if (parseNumber(pl.operandToken, numVal))
            {
                if (numVal < INT24_MIN_VAL || numVal > INT24_MAX_VAL)
                {
                    diagnostics.push_back({lineNum, false, "operand out of range (" + pl.operandToken + ")"});
                    hasErrors = true;
                }
                val = static_cast<int32_t>(numVal);
            }
            else if (isValidLabelName(pl.operandToken))
            {
                auto it = symbolTable.find(pl.operandToken);
                if (it == symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "no such label '" + pl.operandToken + "'"});
                    hasErrors = true;
                }
                else
                {
                    it->second.isUsed = true;
                    val = it->second.value;
                    if (val < INT24_MIN_VAL || val > INT24_MAX_VAL)
                    {
                        diagnostics.push_back({lineNum, false, "operand out of range (" + pl.operandToken + ")"});
                        hasErrors = true;
                    }
                }
            }
            word = ((static_cast<uint32_t>(val) & OPERAND24_MASK) << OPERAND_SHIFT) |
                   (static_cast<uint32_t>(inst->opcode) & OPCODE_MASK);
        }
        else if (inst->opType == OperandType::OFFSET)
        {
            int64_t numVal = 0;
            int32_t offset = 0;
            if (parseNumber(pl.operandToken, numVal))
            {
                // Numeric operand for branch is used as raw offset directly
                if (numVal < INT24_MIN_VAL || numVal > INT24_MAX_VAL)
                {
                    diagnostics.push_back({lineNum, false, "operand out of range (" + pl.operandToken + ")"});
                    hasErrors = true;
                }
                offset = static_cast<int32_t>(numVal);
                rawOffset = offset;
            }
            else if (isValidLabelName(pl.operandToken))
            {
                auto it = symbolTable.find(pl.operandToken);
                if (it == symbolTable.end())
                {
                    diagnostics.push_back({lineNum, false, "no such label '" + pl.operandToken + "'"});
                    hasErrors = true;
                }
                else
                {
                    it->second.isUsed = true;
                    targetLabel = pl.operandToken;
                    int32_t targetAddr = it->second.value;
                    offset = targetAddr - static_cast<int32_t>(pc + 1);
                    rawOffset = offset;
                    if (offset < INT24_MIN_VAL || offset > INT24_MAX_VAL)
                    {
                        diagnostics.push_back({lineNum, false, "operand out of range (" + pl.operandToken + ")"});
                        hasErrors = true;
                    }
                }
            }
            word = ((static_cast<uint32_t>(offset) & OPERAND24_MASK) << OPERAND_SHIFT) |
                   (static_cast<uint32_t>(inst->opcode) & OPCODE_MASK);
        }

        AssembledWord aw;
        aw.address = pc;
        aw.machineCode = word;
        aw.mnemonic = pl.mnemonic;
        aw.operandStr = pl.operandToken;
        aw.labelsAtAddress = pl.labels;
        aw.isBranch = isBranch;
        aw.targetLabel = targetLabel;
        aw.rawOffset = rawOffset;
        assembledWords.push_back(aw);

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
static bool writeObjectFile(const std::string &filename, const std::vector<AssembledWord> &words)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "Error: Could not open output object file " << filename << std::endl;
        return false;
    }

    for (const AssembledWord &aw : words)
    {
        uint32_t code = aw.machineCode;
        uint8_t bytes[4];
        bytes[0] = static_cast<uint8_t>(code & 0xFF);
        bytes[1] = static_cast<uint8_t>((code >> 8) & 0xFF);
        bytes[2] = static_cast<uint8_t>((code >> 16) & 0xFF);
        bytes[3] = static_cast<uint8_t>((code >> 24) & 0xFF);
        out.write(reinterpret_cast<const char *>(bytes), 4);
    }

    return true;
}

// Writes listing file in the preferred advanced format
static bool writeListingFile(
    const std::string &filename,
    const std::vector<AssembledWord> &words,
    const std::map<std::string, SymbolEntry> &symbolTable)
{
    std::ofstream out(filename);
    if (!out.is_open())
    {
        std::cerr << "Error: Could not open output listing file " << filename << std::endl;
        return false;
    }

    // Build reverse map of address -> labels for branch label resolution
    std::map<int32_t, std::string> addrToLabel;
    for (const auto &pair : symbolTable)
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
        for (const std::string &lbl : aw.labelsAtAddress)
        {
            out << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << aw.address
                << " " << lbl << ":\n";
        }

        // Instruction word line: AAAAAAAA XXXXXXXX mnemonic operand
        out << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << aw.address
            << " " << std::setw(8) << aw.machineCode
            << " " << aw.mnemonic;

        if (!aw.operandStr.empty())
        {
            if (aw.isBranch)
            {
                int32_t targetAddr = static_cast<int32_t>(aw.address + 1) + aw.rawOffset;
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
        out << "\n";
    }

    return true;
}


//argc =  the umber of words in the command line and *argv (vector of those words)
int main(int argc, char *argv[])
{//cerr is unbuffered , flushes the output buffer immedaitely after insertion , so it 
    //appears on the string right away even if the program crashes 
    if (argc < 2)
    {//used argv[0] instead of hardcoding asm so that it can be accessed from someother file path too 
        std::cerr << "Usage: " << argv[0] << " <source.asm>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    //ifstream- a type that represnets file opened for reading 
    std::ifstream inFile(inputPath);
    if (!inFile.is_open())
    {
        std::cerr << "Error: Could not open input file " << inputPath << std::endl;
        return 1;
    }
//we are creaitng a separate data structure for the error message and not just combining into the 
//parsed line strcture because we might have multiple errors in a single error

    std::vector<Diagnostic> diagnostics;
    std::vector<ParsedLine> parsedLines;
    std::string lineStr;
    int lineCounter = 1;

    while (std::getline(inFile, lineStr))
    {
        ParsedLine pl = parseLine(lineStr, lineCounter, diagnostics);
        parsedLines.push_back(pl);
        lineCounter++;
    }
    inFile.close();

    std::map<std::string, SymbolEntry> symbolTable;
    std::vector<AssembledWord> dummyWords;

    // Pass 1: Build symbol table and check basic syntax
    assemblePass(parsedLines, Pass::PASS_1, symbolTable, dummyWords, diagnostics);

    // Pass 2: Code generation and label resolution
    std::vector<AssembledWord> codeWords;
    assemblePass(parsedLines, Pass::PASS_2, symbolTable, codeWords, diagnostics);

    // Sort diagnostics by line number
    std::sort(diagnostics.begin(), diagnostics.end(), [](const Diagnostic &a, const Diagnostic &b)
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
            std::cerr << "Warning: Line " << diag.line << ": " << diag.message << std::endl;
        }
        else
        {
            std::cerr << "Error: Line " << diag.line << ": " << diag.message << std::endl;
            hasErrors = true;
        }
    }

    std::string baseName = getBaseFilename(inputPath);
    std::string objFilename = baseName + ".o";
    std::string lstFilename = baseName + ".lst";

    // Write listing file
    writeListingFile(lstFilename, codeWords, symbolTable);

    // Produce no object file if any error occurred
    if (hasErrors)
    {
        std::cerr << "Assembly failed with errors. No object file generated." << std::endl;
        return 1;
    }

    // Write binary object file
    if (!writeObjectFile(objFilename, codeWords))
    {
        return 1;
    }

    return 0;
}
