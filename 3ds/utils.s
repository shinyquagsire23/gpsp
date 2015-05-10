.global InvalidateEntireInstructionCache
.type InvalidateEntireInstructionCache, %function
InvalidateEntireInstructionCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	bx lr

.global InvalidateEntireDataCache
.type InvalidateEntireDataCache, %function
InvalidateEntireDataCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0
	bx lr

.global svcFlushIcache
.type svcFlushIcache, %function
svcFlushIcache:
   svc 0x2E
   bx lr

.global svcRunKernel
.type svcRunKernel, %function
svcRunKernel:
   svc 0x7B
   bx lr
