; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test06.asm - Sum of first N natural numbers for IITPSx
; Computes sum = 1 + 2 + 3 + ... + 10 = 55 (0x37).
;
; Expected final state:
;   A = 55 (0x37)

br start

n:      data 10
sum:    data 0
i:      data 1

start:
loop:
    ; check if (n + 1) - i <= 0 (terminates when i > n)
    ldc n
    ldnl 0
    adc 1
    ldc i
    ldnl 0
    sub
    brz done
    brlz done

    ; sum = sum + i
    ldc sum
    ldnl 0
    ldc i
    ldnl 0
    add
    ldc sum
    stnl 0

    ; i = i + 1
    ldc i
    ldnl 0
    adc 1
    ldc i
    stnl 0
    br loop

done:
    ldc sum
    ldnl 0
    HALT
