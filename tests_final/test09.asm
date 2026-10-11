; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test09.asm - Division by zero error test for IITPSx emulator
; Intentionally executes division by zero:
;   ldc 42  -> A = 42
;   ldc 0   -> B = 42, A = 0
;   div     -> runtime error (A == 0)
;
; Expected emulator behavior:
;   Reports "Error: Division by zero at PC 2" and terminates with exit code 1.

ldc 42
ldc 0
div
HALT
