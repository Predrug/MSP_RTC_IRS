
;------------------------------------------------------------------------------------------------------------------------------------------------
;			@file interrupt.asm
; 			@author Predrag Mitrovic 2016/0678
; 			@date 24 may 2018
;			@brief Low power real time clock and calendar
;
;			Interrupt routines for Timer A CCR0 and CCR1 are written in assembler code.
;			Reaching CCR0 value Timer A starts counting from zero as it is described for up mode. Every time it reaches CCR0 value interrup occurs.
;			CCR0 ISR is used for 7 segment LED multiplexing
;			Reaching CCR1 value interrupt for CCR1 occurs (if it is enabled) and turns off display which imitates PWM driving the LEDs.
;--------------------------------------------------------------------------------------------------------------------------------------------------
			.cdecls C,LIST,"msp430.h"       		; Include device header file

;--------------------------------------------------------------------------------------------------------------------------------------------------
			.ref 	ucDISPLAY						; Defined in main.c but used in interrupt.asm
			.ref 	uc7SEG

;--------------------------------------------------------------------------------------------------------------------------------------------------
;			Constant sector which includes data for 7 segment LED which are addresable using R12 as offset
;--------------------------------------------------------------------------------------------------------------------------------------------------
			.sect	.const
segtab		.byte	0x7e							; digit 0-9 table
			.byte	0x30
			.byte	0x6d
			.byte	0x79
			.byte	0x33
			.byte	0x5b
			.byte	0x5f
			.byte	0x70
			.byte	0x7f
			.byte	0x7b


			.text

;--------------------------------------------------------------------------------------------------------------------------------------------------
;			Timer A CCR1 ISR: Turning off display for brightness PWM implementation
;--------------------------------------------------------------------------------------------------------------------------------------------------
TA0_ISR_CCR1
			bic.b	#BIT0,&TA0CCTL1    				; clear flag
			bis.b	#BIT0|BIT1 , &P11OUT			; disable display  1 , 2
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			reti

;--------------------------------------------------------------------------------------------------------------------------------------------------
;			Timer A CCR0 ISR: Multiplexing 7 segment display and changing display content
;--------------------------------------------------------------------------------------------------------------------------------------------------
TA0_ISR_CCR0
			push R12
			clr R12

			cmp #0 , &ucDISPLAY
			jz DISPLAY0
			cmp #1 , &ucDISPLAY
			jz DISPLAY1
			cmp #2 , &ucDISPLAY
			jz DISPLAY2
			jmp exit



													; display that shows hours and minutes
DISPLAY0	cmp  #1 , &uc7SEG
			jz DISPLAY01
			cmp #2 , &uc7SEG
			jz DISPLAY02
			cmp #3 , &uc7SEG
			jz DISPLAY03
			cmp #4 , &uc7SEG
			jz DISPLAY04
			jmp exit

													; 7seg LED which shows hours (higher digit)
DISPLAY01	mov #2 , &uc7SEG						; increment 7SEG count
			mov.b &RTCHOUR , R12
			rra R12
			rra R12
			rra R12
			rra R12
			bis.b	#BIT0 , &P11OUT					; disable display  2
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT1,&P11OUT					; turn on display 1
			jmp exit

													; 7seg LED which shows hours (lower digit)
DISPLAY02	mov #3 , &uc7SEG						; increment 7SEG count
			mov.b &RTCHOUR , R12
			bic.b	#BIT4|BIT5 , R12
			bis.b	#BIT1 , &P11OUT					; disable display  1
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT0 , &P11OUT					; turn on display 2
			jmp exit

													; 7seg LED which shows day (higher digit)
DISPLAY03	mov #4 , &uc7SEG						; increment 7SEG count
			mov.b &RTCMIN , R12
			rra R12
			rra R12
			rra R12
			rra R12
			bis.b	#BIT0|BIT1 , &P11OUT			; disable display  1,2
			bis.b	#BIT6 , &P10OUT					; disable display  4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT7 , &P10OUT					; turn on display 3
			jmp exit

													; 7seg LED which shows minutes (lower digit)
DISPLAY04	mov #1 , &uc7SEG						; increment 7SEG count
			mov.b &RTCMIN , R12
			bic.b	#BIT4|BIT5|BIT6 , R12
			bis.b	#BIT1|BIT0 , &P11OUT			; disable display  1, 2
			bis.b	#BIT7 , &P10OUT					; disable display 3
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT6 , &P10OUT					; turn on display 4
			jmp exit



													; display that shows days and months
DISPLAY1	cmp #1 , &uc7SEG
			jz DISPLAY11
			cmp #2 , &uc7SEG
			jz DISPLAY12
			cmp #3 , &uc7SEG
			jz DISPLAY13
			cmp #4 , &uc7SEG
			jz DISPLAY14
			jmp exit


													; 7seg LED which shows day (higher digit)
DISPLAY11	mov #2 , &uc7SEG						; increment 7SEG count
			mov.b &RTCDAY , R12
			rra R12
			rra R12
			rra R12
			rra R12
			bis.b	#BIT0 , &P11OUT					; disable display  2
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT1,&P11OUT					; turn on display 1
			jmp exit

													; 7seg LED which shows day (lower digit)
DISPLAY12	mov #3 , &uc7SEG						; increment 7SEG count
			mov.b &RTCDAY , R12
			bic.b	#BIT4|BIT5 , R12
			bis.b	#BIT1 , &P11OUT					; disable display  1
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT0 , &P11OUT					; turn on display 2
			jmp exit

													; 7seg LED which shows month (higher digit)
DISPLAY13	mov #4 , &uc7SEG						; increment 7SEG count
			mov.b &RTCMON , R12
			rra R12
			rra R12
			rra R12
			rra R12
			bis.b	#BIT0|BIT1 , &P11OUT			; disable display  1,2
			bis.b	#BIT6 , &P10OUT					; disable display  4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT7 , &P10OUT					; turn on display 3
			jmp exit

													; 7seg LED which shows month (lower digit)
DISPLAY14	mov #1 , &uc7SEG						; increment 7SEG count
			mov.b &RTCMON , R12
			bic.b	#BIT4 , R12
			bis.b	#BIT1|BIT0 , &P11OUT			; disable display  1, 2
			bis.b	#BIT7 , &P10OUT					; disable display 3
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT6 , &P10OUT					; turn on display 4
			jmp exit



													; display that shows years
DISPLAY2	cmp #1 , &uc7SEG
			jz DISPLAY21
			cmp #2 , &uc7SEG
			jz DISPLAY22
			cmp #3 , &uc7SEG
			jz DISPLAY23
			cmp #4 , &uc7SEG
			jz DISPLAY24
			jmp exit

													; 7seg LED which shows years (millenniums)
DISPLAY21	mov #2 , &uc7SEG						; increment 7SEG count
			mov.b &RTCYEARH , R12
			rra R12
			rra R12
			rra R12
			rra R12
			bis.b	#BIT0 , &P11OUT					; disable display  2
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT1,&P11OUT					; turn on display 1
			jmp exit

													; 7seg LED which shows years (centuries)
DISPLAY22	mov #3 , &uc7SEG						; increment 7SEG count
			mov.b &RTCYEARH , R12
			bic.b	#BIT4|BIT5|BIT6 , R12
			bis.b	#BIT1 , &P11OUT					; disable display  1
			bis.b	#BIT6|BIT7 , &P10OUT			; disable display 3 , 4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT0 , &P11OUT					; turn on display 2
			jmp exit

													; 7seg LED which shows years (decades)
DISPLAY23	mov #4 , &uc7SEG						; increment 7SEG count
			mov.b &RTCYEARL , R12
			rra R12
			rra R12
			rra R12
			rra R12
			bis.b	#BIT0|BIT1 , &P11OUT			; disable display  1,2
			bis.b	#BIT6 , &P10OUT					; disable display  4
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT7 , &P10OUT					; turn on display 3
			jmp exit

													; 7seg LED which shows years (lowest integer)
DISPLAY24	mov #1 , &uc7SEG						; increment 7SEG count
			mov.b &RTCYEARL , R12
			bic.b	#BIT4|BIT5|BIT6|BIT7 , R12
			bis.b	#BIT1|BIT0 , &P11OUT			; disable display  1, 2
			bis.b	#BIT7 , &P10OUT					; disable display 3
			mov.b	segtab(R12),&P6OUT				; index into the table
			bic.b	#BIT6 , &P10OUT					; turn on display 4
			jmp exit


exit		pop R12
			reti

;--------------------------------------------------------------------------------------------------------------------------------------------------
; 			Interrupt Vectors
;--------------------------------------------------------------------------------------------------------------------------------------------------
			.sect .int54
			.short TA0_ISR_CCR0

			.sect .int53
			.short TA0_ISR_CCR1
			.end
