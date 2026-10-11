; Name: Yashashwani | ID: 2501AI49
; "I declare this is my own work."
; test01.asm - Verification test from IITPSx specification
; Tests basic instruction assembly, negative/positive immediate values,
; backward/forward branch offsets, and data directive.
;
; Expected values:
;   addr 0: ldc 0        -> 00000000
;   addr 1: ldc -5       -> FFFFFB00
;   addr 2: ldc +5       -> 00000500
;   addr 3: loop: br loop-> FFFFFF11
;   addr 4: br next      -> 00000011
;   addr 5: next:ldc loop-> 00000300
;   addr 6: ldc var1     -> 00000700
;   addr 7: var1: data 0 -> 00000000

ldc 0
ldc -5
ldc +5
loop: br loop
br next
next: ldc loop
ldc var1
var1: data 0
