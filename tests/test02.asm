; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test02.asm - Comprehensive error-test suite for IITPSx assembler
; Tests all required errors and warnings, one per line.
; Expected: Assembler reports all errors, gives nonzero exit code, and emits no object file.

; Line 8: bogus label name (starts with a digit)
0badlabel: ldc 5

; Line 11: unknown mnemonic
fakeopcode 10

; Line 14: missing operand
ldc

; Line 17: unexpected operand
add 42

; Line 20: extra text at end of line
ldc 5, 6

; Line 23: invalid number (non-octal digit with leading 0)
ldc 08

; Line 26: invalid number (malformed hex/alphanumeric suffix)
ldc 08ge

; Line 29: operand out of range (> 24-bit signed max 8388607)
ldc 99999999

; Line 32: undefined label ("no such label")
ldc nonExistentLabel

; Line 35 & 36: duplicate label
duplicateName: ldc 1
duplicateName: ldc 2

; Line 39: unused label (generates warning, not error)
neverUsedLabel: HALT
