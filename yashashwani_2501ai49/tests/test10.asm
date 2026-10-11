; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test10.asm - Factorial of N using loop and mul
; Computes 6! = 6 * 5 * 4 * 3 * 2 * 1 = 720 (0x2D0).
;
; Expected final state:
;   A = 720 (0x2D0)

br start

n:      data 6
fact:   data 1

start:
loop:
    ; check if n <= 1 (i.e. n - 1 <= 0)
    ldc n
    ldnl 0
    adc -1
    brz done
    brlz done

    ; fact = fact * n
    ldc fact
    ldnl 0
    ldc n
    ldnl 0
    mul
    ldc fact
    stnl 0

    ; n = n - 1
    ldc n
    ldnl 0
    adc -1
    ldc n
    stnl 0
    br loop

done:
    ldc fact
    ldnl 0
    HALT
