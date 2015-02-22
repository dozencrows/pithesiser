@ Pithesiser - a software synthesiser for Raspberry Pi
@ Copyright (C) 2015 Nicholas Tuckett
@ 
@ This program is free software: you can redistribute it and/or modify
@ it under the terms of the GNU General Public License as published by
@ the Free Software Foundation, either version 3 of the License, or
@ (at your option) any later version.
@ 
@ This program is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@ GNU General Public License for more details.
@ 
@ You should have received a copy of the GNU General Public License
@ along with this program.  If not, see <http:@www.gnu.org/licenses/>.

.globl	copy_mono_to_stereo_asm
.globl	mixdown_mono_to_stereo_asm

@ Note there are several implementations illustrating different levels of
@ optimisation, left in for reference.

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
copy_mono_to_stereo_asm_hwpacked_unroll:
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

@	use strd to write & increment, with one unroll & halfword packing
copy_mono_to_stereo_asm:
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10}
	cmp		r3, #1
	ldr		r10, [sp, #DEST_PTR]
	ble		.exit4

.loop4:
	ldsh	r4, [r0], #2			@ read sample 1
	subs	r3, r3, #2				@ adjust count now & set flags - check much later, hides result latency!
	ldsh	r8, [r0], #2			@ read sample 2
	mul		r5, r4, r1				@ sample 1 * left
	mul		r6, r4, r2				@ sample 1 * right
	mul		r9, r8, r1				@ same for sample 2
	mul		r7, r8, r2
	mov		r6, r6, asl #1			@ adust R samples for packing & implicit divide (multiply will have shifted it up)
	mov		r7, r7, asl #1
	pkhtb	r6, r6, r5, asr #15		@ pack sample 1 L & R into r6, R high L low word), dividing L by the shift.
	pkhtb	r7, r7, r9, asr #15		@ sample for sample 2 into r7
	strd	r6, [r10], #8			@ write r6 & r7 to memory - little endian, so sample memory order is L1 R1 L2 R2
    bgt		.loop4

.exit4:
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10}
	bx		lr

@ Basic mixdown mono to stereo
	.set	DEST_PTR,	20
mixdown_mono_to_stereo_asm_basic:
	stmfd	sp!, {r4, r5, r6, r7, r8}
	cmp		r3, #0
	ldr		r7, [sp, #DEST_PTR]
	ble		.exit5

.loop5:
	ldsh	r4, [r0], #2			@ read sample
	ldr		r8, [r7]				@ read destination samples: R high, L low in r8
	subs	r3, r3, #1				@ adjust count now & set flags - check much later, hides result latency!
	mul		r6, r4, r2				@ sample * right
	mul		r5, r4, r1				@ sample * left
	mov		r6, r6, asl #1			@ adust R sample for packing & implicit divide (multiply will have shifted it up)
	pkhtb	r6, r6, r5, asr #15		@ pack sample L & R into r6, R high L low word), dividing L by the shift.
	qadd16	r6, r6, r8				@ add destination samples with saturation to 16 bit.
	str		r6, [r7]				@ store result - little endian, so sample memory order is L R
	add		r7, r7, #4
    bgt		.loop5

.exit5:
	ldmfd	sp!, {r4, r5, r6, r7, r8}
	bx		lr

@ Mixdown with one loop unroll
	.set	DEST_PTR,	36
mixdown_mono_to_stereo_asm:
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12}
	cmp		r3, #0
	ldr		r10, [sp, #DEST_PTR]
	ble		.exit6

.loop6:
	ldsh	r4, [r0], #2			@ read sample 1
	ldr		r11, [r10]				@ read dest sample pair 1
	subs	r3, r3, #2				@ adjust count now & set flags - check much later, hides result latency!
	ldsh	r8, [r0], #2			@ read sample 2
	ldr		r12, [r10, #4]			@ read dest sample pair 2
	mul		r5, r4, r1				@ sample 1 * left
	mul		r6, r4, r2				@ sample 1 * right
	mul		r9, r8, r1				@ same for sample 2
	mul		r7, r8, r2
	mov		r6, r6, asl #1			@ adust R samples for packing & implicit divide (multiply will have shifted it up)
	mov		r7, r7, asl #1
	pkhtb	r6, r6, r5, asr #15		@ pack sample 1 L & R into r6, R high L low word), dividing L by the shift.
	pkhtb	r7, r7, r9, asr #15		@ sample for sample 2 into r7
	qadd16	r6, r6, r11				@ add in destination sample pair 1 with saturation to 16 bit.
	qadd16	r7, r7, r12
	strd	r6, [r10], #8			@ write r6 & r7 to memory - little endian, so sample memory order is L1 R1 L2 R2
    bgt		.loop6

.exit6:
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12}
	bx		lr
