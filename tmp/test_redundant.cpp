// Differential / invariant test for the allegedly redundant leading-whitespace
// skip before mnemonic extraction in parseLine (asm.cpp:294-301).
// It replicates the exact preamble of parseLine (stripComment + trim +
// label loop + empty check) and asserts the invariant that makes the
// skip a no-op: if cleaned is non-empty here, cleaned[0] is never ' '/'\t'
// and idx>=size is never true.
#include <iostream>
#include <string>
#include <vector>
#include <cctype>

static std::string trim(const std::string &str) {
    const char *whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}
static std::string stripComment(const std::string &line) {
    size_t pos = line.find(';');
    if (pos != std::string::npos) return line.substr(0, pos);
    return line;
}

// Returns cleaned string at the point just before "// Extract mnemonic".
// second return value = would the redundant block have done anything?
static std::string preamble(const std::string &rawLine, bool &redundantWouldTrigger) {
    std::string cleaned = trim(stripComment(rawLine));
    if (cleaned.empty()) { redundantWouldTrigger = false; return cleaned; }
    while (true) {
        size_t colonPos = cleaned.find(':');
        if (colonPos == std::string::npos) break;
        cleaned = trim(cleaned.substr(colonPos + 1));
        if (cleaned.empty()) break;
    }
    if (cleaned.empty()) { redundantWouldTrigger = false; return cleaned; }
    // --- start of allegedly redundant block ---
    size_t idx = 0;
    size_t idxAfter = idx;
    while (idxAfter < cleaned.size() && (cleaned[idxAfter] == ' ' || cleaned[idxAfter] == '\t'))
        idxAfter++;
    bool wouldSkip = (idxAfter != idx);
    bool wouldEarlyReturn = (idxAfter >= cleaned.size());
    // --- end ---
    redundantWouldTrigger = (wouldSkip || wouldEarlyReturn);
    return cleaned;
}

int main() {
    std::vector<std::string> cases = {
        "", " ", "\t", "   \t  ", "; comment only", "   ; comment with leading ws",
        "ldc 17", "   ldc 17", "\t\tldc 17", "   \t ldc \t 17  ",
        "ldc", "   ldc", "\tldc\t", "add", "   add   ",
        "loop: ldc 5", "   loop: ldc 5", "\tloop:\tldc 5", "loop:   ldc 5",
        "loop:ldc 5", "a:b:c: ldc 1", "  a:  b:  c:   ldc 1  ",
        "lbl:", "   lbl:   ", "\tlbl:\t", "lbl:   ; comment after label",
        "lbl: ; comment", "   ;lbl: in comment", "ldc 17 ; trailing comment",
        "   ldc 17 ; trailing", "loop: ; comment only after colon",
        ": atypical", " : ", "foo : bar : ldc 0", "x: y: z:",
        "   \r\n", "ldc\t17", "ldc,17", "   ldc,17",
        "HALT", "   HALT   ", "data 0x10", "   data   0x10  ",
        "mylabel: SET 42", "   mylabel:   SET   42   ",
        "br loop", "   br   loop   ", "call func", "\tcall\tfunc",
    };
    int failures = 0;
    for (size_t i = 0; i < cases.size(); ++i) {
        bool triggered = false;
        std::string cleaned = preamble(cases[i], triggered);
        // Invariant check: non-empty cleaned must not start with space/tab
        bool invariantViolated = false;
        if (!cleaned.empty() && (cleaned[0] == ' ' || cleaned[0] == '\t'))
            invariantViolated = true;
        std::cout << (triggered || invariantViolated ? "FAIL" : "pass")
                  << " case[" << i << "] raw=\"" << cases[i] << "\""
                  << " cleaned=\"" << cleaned << "\""
                  << " redundantWouldTrigger=" << triggered << "\n";
        if (triggered || invariantViolated) failures++;
    }
    std::cout << (failures == 0 ? "ALL_PASS: redundant block never triggers\n"
                                : "FAILURES: redundant block triggered\n");
    return failures == 0 ? 0 : 1;
}
