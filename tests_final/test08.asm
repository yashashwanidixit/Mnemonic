; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test08.asm - Mean of an array of numbers using integer division
; Computes the arithmetic mean of an array:
; Elements: 15, 25, 35, 45, 55 (5 elements)
; Sum: 15 + 25 + 35 + 45 + 55 = 175
; Mean = Sum / N = 175 / 5 = 35 (0x23)
;
; Expected final state:
;   A = 35 (0x23)

br start

n:      data 5
arr:    data 15
        data 25
        data 35
        data 45
        data 55

sum:    data 0
idx:    data 0
mean:   data 0

start:
    ; idx = 0, sum = 0
    ldc 0
    ldc idx
    stnl 0

    ldc 0
    ldc sum
    stnl 0

loop:
    ; check if n - idx <= 0
    ldc n
    ldnl 0
    ldc idx
    ldnl 0
    sub
    brz calcMean
    brlz calcMean

    ; A = arr + idx
    ldc idx
    ldnl 0
    ldc arr
    add
    ; A = mem[arr + idx]
    ldnl 0

    ; sum = sum + mem[arr + idx]
    ldc sum
    ldnl 0
    add
    ldc sum
    stnl 0

    ; idx = idx + 1
    ldc idx
    ldnl 0
    adc 1
    ldc idx
    stnl 0
    br loop

calcMean:
    ; To compute sum / n using div:
    ; div computes A := B / A
    ; So B must be sum, and A must be n
    ldc sum
    ldnl 0
    ldc n
    ldnl 0
    div
    ; Now A = sum / n = 35

    ldc mean
    stnl 0

    ldc mean
    ldnl 0
    HALT
