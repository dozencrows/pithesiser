.globl	copy_mono_to_stereo_asm

@ r0	= source ptr
@ r1	= left channel factor (0-32768)
@ r2	= right channel factor (0-32768)
@ r3	= sample count
@ [sp+0]= dest ptr

@ basic version
	.set	DEST_PTR,	16
copy_mono_to_stereo_asm_basic:
	stmfd	sp!, {r4, r5, r6, r7}
	cmp		r3, #0
	ldr		r7, [sp, #DEST_PTR]
	ble		.exit0

.loop0:
	ldsh	r4, [r0], #2
	subs	r3, r3, #1
	mul		r5, r4, r1
	mul		r6, r4, r2
	mov		r5, r5, asr #15
	mov		r6, r6, asr #15
	strh	r5, [r7, #0]
	strh	r6, [r7, #2]
	add		r7, r7, #4
    bgt		.loop0

.exit0:
	ldmfd	sp!, {r4, r5, r6, r7}
	bx		lr

@ using halfword packing to save a memory write
	.set	DEST_PTR,	16
copy_mono_to_stereo_asm_hwpacked:
	stmfd	sp!, {r4, r5, r6, r7}
	cmp		r3, #0
	ldr		r7, [sp, #DEST_PTR]
	ble		.exit1

.loop1:
	ldsh	r4, [r0], #2
	subs	r3, r3, #1
	mul		r6, r4, r2
	mul		r5, r4, r1
	mov		r6, r6, asl #1
	pkhtb	r6, r6, r5, asr #15
	str		r6, [r7]
	add		r7, r7, #4
    bgt		.loop1

.exit1:
	ldmfd	sp!, {r4, r5, r6, r7}
	bx		lr

@ unrolling basic loop once
	.set	DEST_PTR,	28
copy_mono_to_stereo_asm_basic_unroll:
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10}
	cmp		r3, #1
	ldr		r7, [sp, #DEST_PTR]
	ble		.exit2

.loop2:
	ldsh	r4, [r0], #2
	subs	r3, r3, #2
	ldsh	r8, [r0], #2
	mul		r5, r4, r1
	mul		r9, r8, r1
	mul		r6, r4, r2
	mul		r10, r8, r2
	mov		r5, r5, asr #15
	mov		r9, r9, asr #15
	mov		r6, r6, asr #15
	mov		r10, r10, asr #15
	strh	r5, [r7, #0]
	strh	r6, [r7, #2]
	strh	r9, [r7, #4]
	strh	r10, [r7, #6]
	add		r7, r7, #8
    bgt		.loop2

.exit2:
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10}
	bx		lr

@ unrolling halfword packed loop once
	.set	DEST_PTR,	28
copy_mono_to_stereo_asm:
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10}
	cmp		r3, #1
	ldr		r7, [sp, #DEST_PTR]
	ble		.exit3

.loop3:
	ldsh	r4, [r0], #2
	subs	r3, r3, #2
	ldsh	r8, [r0], #2
	mul		r5, r4, r1
	mul		r6, r4, r2
	mul		r9, r8, r1
	mul		r10, r8, r2
	mov		r6, r6, asl #1
	mov		r10, r10, asl #1
	pkhtb	r6, r6, r5, asr #15
	pkhtb	r10, r10, r9, asr #15
	str		r6, [r7]
	str		r10, [r7, #4]
	add		r7, r7, #8
    bgt		.loop3

.exit3:
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10}
	bx		lr
