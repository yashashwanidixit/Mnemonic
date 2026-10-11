; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test04.asm - Arithmetic operations test (mul and div) for IITPSx
; Tests multiplication and division semantics:
;   ldc 6   -> B = 0, A = 6
;   ldc 7   -> B = 6, A = 7
;   mul     -> A = B * A = 6 * 7 = 42
;   ldc 2   -> B = 42, A = 2
;   div     -> A = B / A = 42 / 2 = 21
;   HALT
;
; Expected final state: A = 21 (0x15).

ldc 6
ldc 7
mul
ldc 2
div
HALT
