; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test05.asm - Bubble Sort of an array using IITPSx assembly
; Sorts an array of 6 integers in ascending order:
; Initial array:  45, 12, 89, 34, 23, 7
; Expected array:  7, 12, 23, 34, 45, 89
;
; Uses ldnl/stnl for pointer-based array access and sub + brlz for comparisons.
; Verified via emulator memory dump (-dump).

br start

; Data definitions
n:      data 6
arr:    data 45
        data 12
        data 89
        data 34
        data 23
        data 7

i:      data 0
j:      data 0
lim:    data 0
curp:   data 0
elem1:  data 0
elem2:  data 0

start:
    ; i = 0
    ldc 0
    ldc i
    stnl 0

outerLoop:
    ; check if (n - 1) - i <= 0
    ldc n
    ldnl 0
    adc -1
    ldc i
    ldnl 0
    sub
    brz doneSort
    brlz doneSort

    ; lim = (n - 1) - i
    ldc n
    ldnl 0
    adc -1
    ldc i
    ldnl 0
    sub
    ldc lim
    stnl 0

    ; j = 0
    ldc 0
    ldc j
    stnl 0

innerLoop:
    ; check if lim - j <= 0
    ldc lim
    ldnl 0
    ldc j
    ldnl 0
    sub
    brz nextI
    brlz nextI

    ; curp = arr + j
    ldc j
    ldnl 0
    ldc arr
    add
    ldc curp
    stnl 0

    ; elem1 = mem[curp + 0]
    ldc curp
    ldnl 0
    ldnl 0
    ldc elem1
    stnl 0

    ; elem2 = mem[curp + 1]
    ldc curp
    ldnl 0
    ldnl 1
    ldc elem2
    stnl 0

    ; compare elem1 and elem2: elem2 - elem1 < 0 implies elem1 > elem2
    ldc elem2
    ldnl 0
    ldc elem1
    ldnl 0
    sub
    brlz doSwap
    br noSwap

doSwap:
    ; mem[curp + 0] = elem2
    ldc elem2
    ldnl 0
    ldc curp
    ldnl 0
    stnl 0

    ; mem[curp + 1] = elem1
    ldc elem1
    ldnl 0
    ldc curp
    ldnl 0
    stnl 1

noSwap:
    ; j = j + 1
    ldc j
    ldnl 0
    adc 1
    ldc j
    stnl 0
    br innerLoop

nextI:
    ; i = i + 1
    ldc i
    ldnl 0
    adc 1
    ldc i
    stnl 0
    br outerLoop

doneSort:
    HALT
