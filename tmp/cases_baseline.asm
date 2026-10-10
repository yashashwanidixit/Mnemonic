; whitespace / label edge cases for end-to-end assembler equivalence
   ldc 5
	ldc 6
   	add
loop:   ldc 7
   loop2:	ldc 8
a: b: c:   ldc 9
lblonly:
   lblonly2:
; comment only line

    	
   br lblonly
   HALT
data 0x10
   data   42
mylabel: SET 99
   ldc mylabel
