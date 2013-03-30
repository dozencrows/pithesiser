.globl	filter_apply_asm

@ r0:	sample_data
@ r1:	sample_count
@ r2:	filter_state

@ sample		r3
@ coeff			r4
@ new_sample	r5
@ history		r6
@ round_value	r7
@ precision		14

		.set	ROUNDING_OFFSET, 8192
		.set	PRECISION, 14

filter_apply_asm:
		stmfd	sp!, {r4, r5, r6, r7, lr}

		ldr		r7,=ROUNDING_OFFSET

.L1:	ldrsh	r3,[r0]
		ldr		r4,[r2]									@ new_sample = sample * ic[0]
		mul		r5,r4,r3

		ldr		r4,[r2, #8]								@ new_sample += h[1] * ic[2]
		ldr		r6,[r2, #24]
		mla		r5,r4,r6,r5

		ldr		r4,[r2, #4]								@ new_sample += h[0] * ic[1]
		ldr		r6,[r2, #20]
		mla		r5,r4,r6,r5

		str		r6,[r2, #24]							@ h[1] = h[0]
		str		r3,[r2, #20]							@ h[0] = sample

		ldr		r4,[r2, #16]							@ new_sample += o[1] * oc[1]
		ldr		r6,[r2, #32]
		mla		r5,r4,r6,r5

		ldr		r4,[r2, #12]							@ new_sample += o[0] * oc[0]
		ldr		r6,[r2, #28]
		mla		r5,r4,r6,r5

		add		r5, r7, r5								@ round new_sample
		mov		r5, r5, ASR #PRECISION

		str		r6,[r2, #32]							@ o[1] = o[0]
		str		r5,[r2, #28]							@ o[0] = new_sample

		strh	r5,[r0],#2
		strh	r5,[r0],#2
		subs	r1,r1,#1
		bne		.L1

		ldmfd	sp!, {r4, r5, r6, r7, pc}
