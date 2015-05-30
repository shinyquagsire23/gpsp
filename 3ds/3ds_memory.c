#include <3ds.h>

Result ReprotectMemory(u32 reprotectAddr, u32 reprotectPages, u32 mode, u32* reprotectedPages)
{
	mode=mode&0x7;

	Handle currentHandle;
	svcDuplicateHandle(&currentHandle, 0xFFFF8001);

	u32 ret=0;
	int i; 
	for(i=0; i<reprotectPages && !ret; i++)
		ret = svcControlProcessMemory(currentHandle, reprotectAddr+i*0x1000, 0x0, 0x1000, MEMOP_PROT, mode);

	*reprotectedPages = i;
	return ret;
}
