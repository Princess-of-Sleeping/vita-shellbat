
	.align 4
	.text
	.cpu cortex-a9
	.arch armv7-a
	.syntax unified
	.thumb
	.thumb_func
	.fpu neon

	.global getCpuUsagePercent
	.type   getCpuUsagePercent, %function

@ int getCpuUsagePercent(int core, uint64_t val);

getCpuUsagePercent:
	cmp r3, 0
	bne ret_prev

	// 1000000
	movw r1, #0x4240
	movt r1, #0xF

	cmp r2, r1	// r2 > 1000000
	bhi ret_prev

	// 10000(float)
	movs r3, #0x4000
	movt r3, #0x461C

	subs r2, r1, r2

	vmov s2, r2
	vmov s3, r3

	vcvt.f32.u32 s2, s2

	vdiv.f32 s0, s2, s3
	vcvt.u32.f32 s0, s0

	mov r2, r0 // r2 = core

	vmov r0, s0


	movw r3, #:lower16:prev_clock
	lsls r2, #2
	movt r3, #:upper16:prev_clock

	adds r3, r3, r2
	str r0, [r3]

	bx lr

ret_prev:
	movw r3, #:lower16:prev_clock
	lsls r0, #2
	movt r3, #:upper16:prev_clock

	adds r3, r3, r0
	ldr r0, [r3]

	bx lr

	.data

prev_clock:
	.word 0
	.word 0
	.word 0
	.word 0