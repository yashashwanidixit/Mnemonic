; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test03.asm - SET pseudo-op test for IITPSx assembler
; Tests forward and backward references with the SET directive.
; val is set before use; val2 is set after use.
; Expected execution: A = 75, then A = 75 + 66 = 141.

val: SET 75
ldc val
adc val2
val2: SET 66
HALT
