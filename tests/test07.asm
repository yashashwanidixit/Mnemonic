; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test07.asm - Polynomial Evaluation using Horner's method
; Evaluates P(x) = 3*x^2 + 5*x + 7 for x = 4.
; Horner's form: P(x) = ((3 * x) + 5) * x + 7
; Step 1: 3 * 4 = 12
; Step 2: 12 + 5 = 17
; Step 3: 17 * 4 = 68
; Step 4: 68 + 7 = 75 (0x4B)
;
; Expected final state:
;   A = 75 (0x4B)

br start

x:      data 4
res:    data 0

start:
    ; A = 3
    ldc 3

    ; A = 3 * x
    ldc x
    ldnl 0
    mul

    ; A = (3 * x) + 5
    adc 5

    ; A = ((3 * x) + 5) * x
    ldc x
    ldnl 0
    mul

    ; A = ((3 * x) + 5) * x + 7
    adc 7

    ; Store result in res
    ldc res
    stnl 0

    ; Reload res into A to confirm
    ldc res
    ldnl 0
    HALT
