.globl	filter_apply_asm

@ r0:	sample_data
@ r1:	sample_count
@ r2:	filter_state

		.set	ROUNDING_OFFSET, 8192
		.set	PRECISION, 14

filter_apply_asm:
		stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		sub		sp, #4

		ldr		r8,[r2]									@ ic[0]
		ldr		r9,[r2, #4]								@ ic[1]
		ldr		r10,[r2, #8]							@ ic[2]
		ldr		r11,[r2, #12]							@ oc[0]
		ldr		r12,[r2, #16]							@ oc[1]

		ldr		r3,[r2, #20]							@ h[0]
		ldr		r4,[r2, #24]							@ h[1]
		ldr		r5,[r2, #28]							@ o[0]
		ldr		r6,[r2, #32]							@ o[1]

		str		r2,[sp]
		b		.L1

.L0:
		mov		r4, r3									@ h[1] = h[0]
		mov		r6, r5									@ o[1] = o[0]
		mov		r3, r2									@ h[0] = last input
		mov		r5, r7									@ o[0] = last output

.L1:
		ldrsh	r2,[r0]									@ load new sample
		subs	r1,r1,#1								@ decrement overall count (here to avoid stall from 16-bit load above)
		mul		r7,r10,r4								@ h[1] * ic[2]			  (helps to avoid stall from 16-bit load above)
		mla		r7,r2,r8,r7								@ += sample * ic[0]		  (now use available 16-bit loaded value)
		mla		r7,r3,r9,r7								@ += h[0] * ic[1]
		mla		r7,r5,r11,r7							@ += o[0] * oc[0]
		mla		r7,r6,r12,r7							@ += o[1] * oc[1]

		add		r7, r7, #ROUNDING_OFFSET				@ round new_sample
		mov		r7, r7, asr #PRECISION

		strh	r7,[r0], #2
		strh	r7,[r0], #2

		bne		.L0										@ checks r1 for zero

		ldr		r2, [sp]

		str		r3, [r2, #20]							@ store final history & output state
		str		r4, [r2, #24]
		str		r5, [r2, #28]
		str		r6, [r2, #32]

		add		sp, #4
		ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}
