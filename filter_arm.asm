.globl	filter_apply_asm
.globl	filter_apply_interp_asm
.globl	filter_apply_hp_asm
.globl	filter_apply_interp_hp_asm
.globl	__aeabi_idiv

@ r0:	signed positive numerator
@ r1:	signed positive denominator
idiv:
		neg		r1, r1
		mov		r2, #0
		adds	r0, r0, r0
	.rept	32
		adcs	r2, r1, r2, lsl #1
		subcc	r2, r2, r1
		adcs	r0, r0, r0
	.endr
		mov		pc,lr

@ r0:	sample_data
@ r1:	sample_count
@ r2:	filter_state

		.set	ROUNDING_OFFSET, 8192
		.set	PRECISION, 14

		.set	FS_INPUT_C0, 	0
		.set	FS_INPUT_C1, 	4
		.set	FS_INPUT_C2, 	8
		.set	FS_OUTPUT_C0, 	12
		.set	FS_OUTPUT_C1, 	16
		.set	FS_HISTORY0, 	20
		.set	FS_HISTORY1, 	24
		.set	FS_OUTPUT0, 	28
		.set	FS_OUTPUT1, 	32

filter_apply_asm:
		stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		sub		sp, #4

		ldr		r8,[r2, #FS_INPUT_C0]					@ ic[0]
		ldr		r9,[r2, #FS_INPUT_C1]					@ ic[1]
		ldr		r10,[r2, #FS_INPUT_C2]					@ ic[2]
		ldr		r11,[r2, #FS_OUTPUT_C0]					@ oc[0]
		ldr		r12,[r2, #FS_OUTPUT_C1]					@ oc[1]

		ldr		r3,[r2, #FS_HISTORY0]					@ h[0]
		ldr		r4,[r2, #FS_HISTORY1]					@ h[1]
		ldr		r5,[r2, #FS_OUTPUT0]					@ o[0]
		ldr		r6,[r2, #FS_OUTPUT1]					@ o[1]

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

		ldr		r1, [sp]

		str		r2, [r1, #FS_HISTORY0]					@ store final history & output state
		str		r3, [r1, #FS_HISTORY1]
		str		r7, [r1, #FS_OUTPUT0]
		str		r5, [r1, #FS_OUTPUT1]

		add		sp, #4
		ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}

@ r0:	sample_data
@ r1:	sample_count
@ r2:	filter_state_current
@ r3:	filter_state_last

		.set	S_INTERPOLANT,		0
		.set	S_INTERPOLATE_STEP,	S_INTERPOLANT + 4
		.set	S_INPUT_C0,			S_INTERPOLATE_STEP + 4
		.set	S_INPUT_C1,			S_INPUT_C0 + 4
		.set	S_INPUT_C2,			S_INPUT_C1 + 4
		.set	S_OUTPUT_C0,		S_INPUT_C2 + 4
		.set	S_OUTPUT_C1,		S_OUTPUT_C0 + 4
		.set	S_LAST_INPUT_C0,	S_OUTPUT_C1 + 4
		.set	S_LAST_INPUT_C1,	S_LAST_INPUT_C0 + 4
		.set	S_LAST_INPUT_C2,	S_LAST_INPUT_C1 + 4
		.set	S_LAST_OUTPUT_C0,	S_LAST_INPUT_C2 + 4
		.set	S_LAST_OUTPUT_C1,	S_LAST_OUTPUT_C0 + 4
		.set	S_FRAME_SIZE,		S_LAST_OUTPUT_C1 + 4

		.set	INTERP_PRECISION,	15
		.set	INTERP_ONE,			32768

filter_apply_interp_asm:

		@ save registers & reserve stack space
		stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		sub		sp, #8
		str		r2, [sp]
		str		r3, [sp, #4]

		@ Calculate interpolation step:		(1 << INTERP_PRECISION) / sample_count
		stmfd	sp!, {r0, r1, r2, r3}
		mov		r0, #INTERP_ONE
		bl		__aeabi_idiv
		mov		r12, r0
		ldmfd	sp!, {r0, r1, r2, r3}

		@ Copy state coefficients onto stack	(saves a register in the loop)
		ldr		r4, [r3, #FS_INPUT_C0]
		ldr		r5, [r3, #FS_INPUT_C1]
		ldr		r6, [r3, #FS_INPUT_C2]
		ldr		r7, [r3, #FS_OUTPUT_C0]
		ldr		r8, [r3, #FS_OUTPUT_C1]
		stmfd	sp!, {r4, r5, r6, r7, r8}

		ldr		r4, [r2, #FS_INPUT_C0]
		ldr		r5, [r2, #FS_INPUT_C1]
		ldr		r6, [r2, #FS_INPUT_C2]
		ldr		r7, [r2, #FS_OUTPUT_C0]
		ldr		r8, [r2, #FS_OUTPUT_C1]
		stmfd	sp!, {r4, r5, r6, r7, r8}

		@ Set up history & output into registers for temporary use
		@ Can assume history is same for both last and current
		ldr		r7, [r3, #FS_HISTORY0]							@ lh & h[0]
		ldr		r8, [r3, #FS_HISTORY1]							@ lh & h[1]
		ldr		r9, [r3, #FS_OUTPUT0]							@ lo[0]
		ldr		r10, [r3, #FS_OUTPUT1]							@ lo[1]

		ldr		r5, [r2, #FS_OUTPUT0]							@ o[0]
		ldr		r6, [r2, #FS_OUTPUT1]							@ o[1]

		@ Set interpolation values and start loop
		sub		r11, r11, r11
		stmfd	sp!, {r11, r12}
		b		.L3

.L2:
		@ advance historical data
		mov		r8, r7					@ h[]
		mov		r7, r2

		mov		r10, r9					@ lo[]
		mov		r9, r3

		mov		r6, r5					@ o[]
		mov		r5, r4

.L3:
		@ r0 -> sample data, r1 = sample count, r5 to r10 = historical data
		@ r2 = current sample, r3 = sample from last filter, r4 = sample from current filter
		@ r11, r12 = temp values
		@ r14 could be used... (lr, saved for return)

		ldrsh	r2, [r0]								@ load new sample to r2

		@ carry out last state calculations (to r3)
		@ round result & hold onto it (avoid store if possible)
		ldr		r11, [sp, #S_LAST_INPUT_C2]
		ldr		r12, [sp, #S_LAST_INPUT_C0]
		mul		r3, r8, r11								@ h[1] * ic[2]
		ldr		r11, [sp, #S_LAST_INPUT_C1]
		mla		r3, r2, r12, r3							@ += sample * ic[0]
		ldr		r12, [sp, #S_LAST_OUTPUT_C0]
		mla		r3, r7, r11, r3							@ += h[0] * ic[1]
		ldr		r11, [sp, #S_LAST_OUTPUT_C1]
		mla		r3, r9, r12, r3							@ += lo[0] * oc[0]
		mla		r3, r10, r11, r3						@ += lo[1] * oc[1]

		add		r3, r3, #ROUNDING_OFFSET				@ round new_sample
		mov		r3, r3, asr #PRECISION

		@ carry out current state calculations (to r4)
		@ round result & store it (avoid store if possible)
		ldr		r11, [sp, #S_INPUT_C2]
		ldr		r12, [sp, #S_INPUT_C0]
		mul		r4, r8, r11								@ h[1] * ic[2]
		ldr		r11, [sp, #S_INPUT_C1]
		mla		r4, r2, r12, r4							@ += sample * ic[0]
		ldr		r12, [sp, #S_OUTPUT_C0]
		mla		r4, r7, r11, r4							@ += h[0] * ic[1]
		ldr		r11, [sp, #S_OUTPUT_C1]
		mla		r4, r5, r12, r4							@ += lo[0] * oc[0]
		mla		r4, r6, r11, r4							@ += lo[1] * oc[1]

		add		r4, r4, #ROUNDING_OFFSET				@ round new_sample
		mov		r4, r4, asr #PRECISION

		@ calculate interpolated result and store
		ldr		r11, [sp, #S_INTERPOLANT]
		subs	r1, r1, #1								@ decrement overall count (here to avoid stall from load above)
		mul		r12, r11, r4							@ i * new_sample
		rsb		r11, r11, #INTERP_ONE
		mla		r12, r11, r3, r12						@ += (1 - i) * last_sample
		mov		r12, r12, asr #INTERP_PRECISION
		strh	r12, [r0], #2
		strh	r12, [r0], #2

		@ loop for next sample
		ldr		r12, [sp, #S_INTERPOLATE_STEP]
		rsb		r11, r11, #INTERP_ONE
		add		r11, r12, r11
		str		r11, [sp, #S_INTERPOLANT]
		bne		.L2										@ based on flags set from decrement of r1

		@ write back historical data to state
		add		sp, sp, #S_FRAME_SIZE

		ldr		r1, [sp]
		str		r2,[r1, #FS_HISTORY0]							@ h[0]
		str		r7,[r1, #FS_HISTORY1]							@ h[1]
		str		r4,[r1, #FS_OUTPUT0]							@ o[0]
		str		r5,[r1, #FS_OUTPUT1]							@ o[1]

		ldr		r1, [sp, #4]
		str		r2,[r1, #FS_HISTORY0]							@ lh[0]
		str		r7,[r1, #FS_HISTORY1]							@ lh[1]
		str		r3,[r1, #FS_OUTPUT0]							@ lo[0]
		str		r9,[r1, #FS_OUTPUT1]							@ lo[1]

		@ release stack space, restore registers & return
		add		sp, #8
		ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}

		.set	HP_ROUNDING_OFFSET, 131072
		.set	HP_PRECISION, 18

filter_apply_hp_asm:
		stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		sub		sp, #4

		ldr		r8,[r2, #FS_INPUT_C0]					@ ic[0]
		mov		r8,r8,asl #4
		ldr		r9,[r2, #FS_INPUT_C1]					@ ic[1]
		mov		r9,r9,asl #4
		ldr		r10,[r2, #FS_INPUT_C2]					@ ic[2]
		mov		r10,r10,asl #4
		ldr		r11,[r2, #FS_OUTPUT_C0]					@ oc[0]
		mov		r11,r11,asl #4
		ldr		r12,[r2, #FS_OUTPUT_C1]					@ oc[1]
		mov		r12,r12,asl #4

		ldr		r3,[r2, #FS_HISTORY0]					@ h[0]
		ldr		r4,[r2, #FS_HISTORY1]					@ h[1]
		ldr		r5,[r2, #FS_OUTPUT0]					@ o[0]
		ldr		r6,[r2, #FS_OUTPUT1]					@ o[1]

		str		r2,[sp]
		b		.L5

.L4:
		mov		r4, r3									@ h[1] = h[0]
		mov		r6, r5									@ o[1] = o[0]
		mov		r3, r2									@ h[0] = last input
		mov		r5, r7									@ o[0] = last output

.L5:
		ldrsh	r2,[r0]									@ load new sample
		sub		r1,r1,#1								@ decrement overall count (here to avoid stall from 16-bit load above)
		smull	r7,r14,r10,r4							@ h[1] * ic[2]			  (helps to avoid stall from 16-bit load above)
		smlal	r7,r14,r2,r8							@ += sample * ic[0]		  (now use available 16-bit loaded value)
		smlal	r7,r14,r3,r9							@ += h[0] * ic[1]
		smlal	r7,r14,r5,r11							@ += o[0] * oc[0]
		smlal	r7,r14,r6,r12							@ += o[1] * oc[1]

		adds	r7,r7, #HP_ROUNDING_OFFSET				@ round new_sample
		adc		r14,r14, #0
		mov		r7, r7, lsr #HP_PRECISION
		orr		r7, r14, lsl #32-HP_PRECISION

		strh	r7,[r0], #2
		strh	r7,[r0], #2

		cmp		r1, #0
		bne		.L4										@ checks r1 for zero

		ldr		r1, [sp]

		str		r2, [r1, #FS_HISTORY0]					@ store final history & output state
		str		r3, [r1, #FS_HISTORY1]
		str		r7, [r1, #FS_OUTPUT0]
		str		r5, [r1, #FS_OUTPUT1]

		add		sp, #4
		ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		mov		pc, lr

filter_apply_interp_hp_asm:

		@ save registers & reserve stack space
		stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		sub		sp, #8
		str		r2, [sp]
		str		r3, [sp, #4]

		@ Calculate interpolation step:		(1 << INTERP_PRECISION) / sample_count
		stmfd	sp!, {r0, r1, r2, r3}
		mov		r0, #INTERP_ONE
		bl		__aeabi_idiv
		mov		r12, r0
		ldmfd	sp!, {r0, r1, r2, r3}

		@ Copy state coefficients onto stack	(saves a register in the loop)
		ldr		r4, [r3, #FS_INPUT_C0]
		mov		r4, r4, asl #4
		ldr		r5, [r3, #FS_INPUT_C1]
		mov		r5, r5, asl #4
		ldr		r6, [r3, #FS_INPUT_C2]
		mov		r6, r6, asl #4
		ldr		r7, [r3, #FS_OUTPUT_C0]
		mov		r7, r7, asl #4
		ldr		r8, [r3, #FS_OUTPUT_C1]
		mov		r8, r8, asl #4
		stmfd	sp!, {r4, r5, r6, r7, r8}

		ldr		r4, [r2, #FS_INPUT_C0]
		mov		r4, r4, asl #4
		ldr		r5, [r2, #FS_INPUT_C1]
		mov		r5, r5, asl #4
		ldr		r6, [r2, #FS_INPUT_C2]
		mov		r6, r6, asl #4
		ldr		r7, [r2, #FS_OUTPUT_C0]
		mov		r7, r7, asl #4
		ldr		r8, [r2, #FS_OUTPUT_C1]
		mov		r8, r8, asl #4
		stmfd	sp!, {r4, r5, r6, r7, r8}

		@ Set up history & output into registers for temporary use
		@ Can assume history is same for both last and current
		ldr		r7, [r3, #FS_HISTORY0]							@ lh & h[0]
		ldr		r8, [r3, #FS_HISTORY1]							@ lh & h[1]
		ldr		r9, [r3, #FS_OUTPUT0]							@ lo[0]
		ldr		r10, [r3, #FS_OUTPUT1]							@ lo[1]

		ldr		r5, [r2, #FS_OUTPUT0]							@ o[0]
		ldr		r6, [r2, #FS_OUTPUT1]							@ o[1]

		@ Set interpolation values and start loop
		sub		r11, r11, r11
		stmfd	sp!, {r11, r12}
		b		.L7

.L6:
		@ advance historical data
		mov		r8, r7					@ h[]
		mov		r7, r2

		mov		r10, r9					@ lo[]
		mov		r9, r3

		mov		r6, r5					@ o[]
		mov		r5, r4

.L7:
		@ r0 -> sample data, r1 = sample count, r5 to r10 = historical data
		@ r2 = current sample, r3 = sample from last filter, r4 = sample from current filter
		@ r11, r12 = temp values
		@ r14 could be used... (lr, saved for return)

		ldrsh	r2, [r0]								@ load new sample to r2

		@ carry out last state calculations (to r3)
		@ round result & hold onto it (avoid store if possible)
		ldr		r11, [sp, #S_LAST_INPUT_C2]
		ldr		r12, [sp, #S_LAST_INPUT_C0]
		smull	r3, r14, r8, r11						@ h[1] * ic[2]
		ldr		r11, [sp, #S_LAST_INPUT_C1]
		smlal	r3, r14, r2, r12						@ += sample * ic[0]
		ldr		r12, [sp, #S_LAST_OUTPUT_C0]
		smlal	r3, r14, r7, r11						@ += h[0] * ic[1]
		ldr		r11, [sp, #S_LAST_OUTPUT_C1]
		smlal	r3, r14, r9, r12						@ += lo[0] * oc[0]
		smlal	r3, r14, r10, r11						@ += lo[1] * oc[1]

		adds	r3, r3, #HP_ROUNDING_OFFSET				@ round new_sample
		adc		r14, r14, #0
		mov		r3, r3, lsr #HP_PRECISION
		orr		r3, r14, lsl #32-HP_PRECISION

		@ carry out current state calculations (to r4)
		@ round result & store it (avoid store if possible)
		ldr		r11, [sp, #S_INPUT_C2]
		ldr		r12, [sp, #S_INPUT_C0]
		smull	r4, r14, r8, r11						@ h[1] * ic[2]
		ldr		r11, [sp, #S_INPUT_C1]
		smlal	r4, r14, r2, r12						@ += sample * ic[0]
		ldr		r12, [sp, #S_OUTPUT_C0]
		smlal	r4, r14, r7, r11						@ += h[0] * ic[1]
		ldr		r11, [sp, #S_OUTPUT_C1]
		smlal	r4, r14, r5, r12						@ += lo[0] * oc[0]
		smlal	r4, r14, r6, r11						@ += lo[1] * oc[1]

		adds	r4, r4, #HP_ROUNDING_OFFSET				@ round new_sample
		adc		r14, r14, #0
		mov		r4, r4, lsr #HP_PRECISION
		orr		r4, r14, lsl #32-HP_PRECISION

		@ calculate interpolated result and store
		ldr		r11, [sp, #S_INTERPOLANT]
		subs	r1, r1, #1								@ decrement overall count (here to avoid stall from load above)
		mul		r12, r11, r4							@ i * new_sample
		rsb		r11, r11, #INTERP_ONE
		mla		r12, r11, r3, r12						@ += (1 - i) * last_sample
		mov		r12, r12, asr #INTERP_PRECISION
		strh	r12, [r0], #2
		strh	r12, [r0], #2

		@ loop for next sample
		ldr		r12, [sp, #S_INTERPOLATE_STEP]
		rsb		r11, r11, #INTERP_ONE
		add		r11, r12, r11
		str		r11, [sp, #S_INTERPOLANT]
		bne		.L6										@ based on flags set from decrement of r1

		@ write back historical data to state
		add		sp, sp, #S_FRAME_SIZE

		ldr		r1, [sp]
		str		r2,[r1, #FS_HISTORY0]							@ h[0]
		str		r7,[r1, #FS_HISTORY1]							@ h[1]
		str		r4,[r1, #FS_OUTPUT0]							@ o[0]
		str		r5,[r1, #FS_OUTPUT1]							@ o[1]

		ldr		r1, [sp, #4]
		str		r2,[r1, #FS_HISTORY0]							@ lh[0]
		str		r7,[r1, #FS_HISTORY1]							@ lh[1]
		str		r3,[r1, #FS_OUTPUT0]							@ lo[0]
		str		r9,[r1, #FS_OUTPUT1]							@ lo[1]

		@ release stack space, restore registers & return
		add		sp, #8
		ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
		mov		pc,lr
