
#include "InlineHook.h"


//
// 保存所有HOOK信息表
//
HOOK_ENTRY g_HookTable[HOOK_TABLE_MAX];
ULONG g_HookTableNums;
ERESOURCE g_HookLock;


/**
* info structure:
*	0x26 means: 6 2 (high-low, low-high)
*
* Bit 0:
*	1 - has ModR/M
*	0 - no ModR/M byte
* Bit 1~3:
*	0 - no imm
*	1 - Ib, Jb
*	2 - Iw
*	3 - Iv, Iz, Jz
*	4 - Ib + Iw
*
* special cases:
*	1. group f6xx, f7xx: nnn = 000,001 -- uses Iz
*	2. 9a, ea: Ap (xxxx:xxxxxxxx), 6-byte imm
*	3. Ob, Ov: 4-byte long offset
*/

const uint8_t c_opinfo[256] = {
	/*        0 1  2 3  4 5  6 7  8 9  A B  C D  E F  */
	/*       ---------------------------------------  */
	/* 00 */ 0x11, 0x11, 0x26, 0x00, 0x11, 0x11, 0x26, 0x00,
	/* 10 */ 0x11, 0x11, 0x26, 0x00, 0x11, 0x11, 0x26, 0x00,
	/* 20 */ 0x11, 0x11, 0x26, 0x00, 0x11, 0x11, 0x26, 0x00,
	/* 30 */ 0x11, 0x11, 0x26, 0x00, 0x11, 0x11, 0x26, 0x00,
	/* 40 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 50 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 60 */ 0x00, 0x11, 0x00, 0x00, 0x67, 0x23, 0x00, 0x00,
	/* 70 */ 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	/* 80 */ 0x37, 0x33, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
	/* 90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* A0 */ 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
	/* B0 */ 0x22, 0x22, 0x22, 0x22, 0x66, 0x66, 0x66, 0x66,
	/* C0 */ 0x33, 0x40, 0x11, 0x37, 0x80, 0x40, 0x02, 0x00,
	/* D0 */ 0x11, 0x11, 0x22, 0x00, 0x11, 0x11, 0x11, 0x11,
	/* E0 */ 0x22, 0x22, 0x22, 0x22, 0x66, 0x02, 0x00, 0x00,
	/* F0 */ 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11,
	/*        0 1  2 3  4 5  6 7  8 9  A B  C D  E F  */
	/*       ---------------------------------------  */
	0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, /* 0F 00 */
	0x11, 0x11, 0x11, 0x11, 0x10, 0x00, 0x00, 0x01, /* 0F 10 */
	0x11, 0x11, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, /* 0F 20 */
	0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, /* 0F 30 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* 0F 40 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* 0F 50 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* 0F 60 */
	0x33, 0x11, 0x11, 0x10, 0x00, 0x00, 0x11, 0x11, /* 0F 70 */
	0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, /* 0F 80 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* 0F 90 */
	0x00, 0x01, 0x31, 0x00, 0x00, 0x01, 0x31, 0x11, /* 0F A0 */
	0x11, 0x11, 0x11, 0x11, 0x00, 0x11, 0x11, 0x11, /* 0F B0 */
	0x11, 0x31, 0x33, 0x31, 0x00, 0x00, 0x00, 0x00, /* 0F C0 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* 0F D0 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* 0F E0 */
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x10  /* 0F F0 */
	/*        0 1  2 3  4 5  6 7  8 9  A B  C D  E F  */
	/*       ---------------------------------------  */
};

enum cpu_mode_t { cm_legacy = -1, cm_compat = 0, cm_64bit = 1 };

int LhpHookInstructLen(void* Address)
{
	enum cpu_mode_t cm = cm_compat;
	register uint8_t* p = (uint8_t*)Address;
	register uint8_t b = 0;
	register boolean_t pre_66 = FALSE;
	uint8_t info;
	register int tbl_fixup = 0;
	register uint8_t i_info = 0;
	register uint8_t modrm = 0;

	if (!Address)
		return -1;

	while (1)
	{
		b = *p++;

		if (b >= 0x40 && b <= 0x4f && cm == cm_64bit)
			continue;
		else if (b == 0x66) {
			pre_66 = TRUE;
			continue;
		}
		else if (b == 0xf0 || b == 0xf2 || b == 0xf3 ||
			b == 0x26 || b == 0x2e || b == 0x36 ||
			b == 0x3e || b == 0x64 || b == 0x65 ||
			b == 0x67)
			continue;
		break;
	}

	if (b == 0x0f) {
		b = *p++;
		tbl_fixup = 128;
	}

	info = c_opinfo[(b >> 1) + tbl_fixup];
	info = ((b % 2) ? info : (info >> 4)) & 0x0f;
	i_info = (info >> 1) & 7;

	if (info & 0x01) {
		/* has modrm */
		modrm = *p++;
		do {
			register uint8_t sib = 0;
			boolean_t has_sib = FALSE;
			register uint8_t tmp = 0;

			if ((modrm & 0xc0) == 0xc0)	/* mod == 3 */
				break;

			if (cm != cm_legacy && (modrm & 0x07) == 4) {
				has_sib = TRUE;
				sib = *p++;
			}
			/* displacement */
			tmp = has_sib ? sib : modrm;
			if (!(modrm & 0xc0)) {	/* mod == 00b */
				if ((tmp & 0x07) == 5)
					p += (cm == cm_legacy) ? 2 : 4;
			}
			else if ((modrm & 0xc0) == 0x40) {	/* mod == 01b */
				++p;
			}
			else {	/* mod == 0x10b */
				p += (cm == cm_legacy) ? 2 : 4;
			}
		} while (0);
	}

	/* special cases */
	do
	{
		register uint8_t tmp = (modrm & 0x38) >> 3;	/* nnn */
		if (tmp == 0 || tmp == 1) {
			if (b == 0xf6)
				i_info |= 1;
			else if (b == 0xf7)
				i_info |= 3;	/* Iz */
		}
		if (b == 0x9a || b == 0xea)	/* Ap */
			p += 6;
		if (b >= 0xa0 && b <= 0xa3)
			p += 4;

	} while (0);

	/* take care of immediate value */
	switch (i_info) {
	case 0:	break;
	case 1:	++p; break;
	case 2:	p += 2;	break;
	case 3:	p += pre_66 ? 2 : 4; break;
	case 4:	p += 3;	break;
	}

	return (int)(p - (uint8_t*)Address);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
NTSTATUS LhpHookModuleInit()
{
	RtlZeroMemory(g_HookTable, sizeof(g_HookTable));
	g_HookTableNums = 0;
	ExInitializeResourceLite(&g_HookLock);

	return STATUS_SUCCESS;
}

VOID LhpHookModuleUninit()
{
	ULONG Index = 0;

	//
	// 恢复所有hook
	//
	ExAcquireResourceExclusiveLite(&g_HookLock, TRUE);

	for (Index = 0; Index < g_HookTableNums; Index++)
	{
		LhpHookUninstallHook(g_HookTable[Index].HookId);
	}

	ExReleaseResourceLite(&g_HookLock);
	ExDeleteResourceLite(&g_HookLock);
}


PVOID LhpHookGetOriginFunAddress(HOOKID HookId)
{
	ULONG Index = 0;

	if (HookId == INVALID_HOOKID)
	{
		return NULL;
	}

	for (Index = 0; Index < g_HookTableNums; Index++)
	{
		if (g_HookTable[Index].HookId == HookId)
		{
			return g_HookTable[Index].OriginFunction;
		}
	}

	return NULL;
}


VOID LhpHookInitHookStub(PHOOK_STUB HookStub)
{
	if (HookStub == NULL)
	{
		return;
	}

	RtlZeroMemory(HookStub, sizeof(HOOK_STUB));
	HookStub->jz[0] = 0x74;
	HookStub->jz[1] = 0x06;
	HookStub->jnz[0] = 0x75;
	HookStub->jnz[1] = 0x04;
	memcpy(HookStub->flag, "xuan", sizeof(HookStub->flag));
	memset(HookStub->nop, 0x90, sizeof(HookStub->nop));
	HookStub->jmp_handler[0] = 0xE9;
	memset(HookStub->origin_code, 0x90, sizeof(HookStub->origin_code));
	HookStub->jmp_origin_fun[0] = 0xE9;
	memcpy(HookStub->flag2, "yuan", sizeof(HookStub->flag2));
}


NTSTATUS LhpHookSetMemoryPageExecute(PVOID TargetAddress, ULONG Length)
{
	PMDL mdl = IoAllocateMdl(TargetAddress, Length, FALSE, FALSE, NULL);
	MmBuildMdlForNonPagedPool(mdl);
	MmProbeAndLockPages(mdl, KernelMode, IoWriteAccess);
	MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, 0, NormalPagePriority);
	MmProtectMdlSystemAddress(mdl, PAGE_EXECUTE_READWRITE);

	return STATUS_SUCCESS;
}


HOOKID LhpHookInstallHook(PVOID TargetAddress, PVOID HandlerAddress)
{
	HOOKID newHookId = 0;
	PHOOK_STUB newHookStub = NULL;
	int hook_len = 0;
	uint8_t* start_address = NULL;
	byte jmp_entry[20] = { 0 };
	DWORD CR0_VALUE = 0;

	if (TargetAddress == NULL || HandlerAddress == NULL || g_HookTableNums >= HOOK_TABLE_MAX)
	{
		return INVALID_HOOKID;
	}

	//
	// 计算要HOOK的字节长度
	//
	start_address = (uint8_t*)TargetAddress;
	while (hook_len < 5)
	{
		int instruct_len = LhpHookInstructLen(start_address);
		if (instruct_len <= 0 || instruct_len >= 10)
			break;

		hook_len += instruct_len;
		start_address += instruct_len;
	}

	//
	// 分配中间跳指令区，并设置可执行属性
	//
	newHookStub = ExAllocatePool(NonPagedPool, sizeof(HOOK_STUB));
	if (newHookStub == NULL)
	{
		return INVALID_HOOKID;
	}

	//
	// 设置分配的内存可执行属性
	//
	LhpHookSetMemoryPageExecute(newHookStub, sizeof(HOOK_STUB));


	//
	// 准备hook中间跳转指令
	//
	LhpHookInitHookStub(newHookStub);
	memcpy(newHookStub->origin_code, TargetAddress, hook_len);
	*((unsigned long *)(&newHookStub->jmp_handler[1])) = (unsigned long)HandlerAddress - ((unsigned long)&newHookStub->jmp_handler + 5);
	*((unsigned long *)(&newHookStub->jmp_origin_fun[1])) = ((unsigned long)TargetAddress + hook_len) - ((unsigned long)&newHookStub->jmp_origin_fun + 5);


	//
	// 准备Hook目标入口跳转指令
	//
	memset(jmp_entry, 0x90, sizeof(jmp_entry));
	jmp_entry[0] = 0xE9;
	*((unsigned long*)(&jmp_entry[1])) = (unsigned long)newHookStub - ((unsigned long)TargetAddress + 5);

	//
	// 开始安装钩子
	//
	// 关写保护
	//
	_asm
	{  
		//  
		// 关写保护，HOOK时的标准动作
		//
		cli
		mov   eax, cr0 
		mov   CR0_VALUE, eax
		and   eax, not 10000H 
		mov   cr0, eax   
	}

	memcpy(TargetAddress, jmp_entry, hook_len);

	//
	// 开写保护
	//
	_asm
	{
		mov     eax, CR0_VALUE                  
		mov     cr0, eax
		sti
	}  


	//
	// 记录本次HOOK
	//
	ExAcquireResourceExclusiveLite(&g_HookLock, TRUE);

	newHookId = 0x1000 + g_HookTableNums;
	g_HookTable[g_HookTableNums].HookStub = newHookStub;
	g_HookTable[g_HookTableNums].HookId = newHookId;
	g_HookTable[g_HookTableNums].TargetAddress = TargetAddress;
	g_HookTable[g_HookTableNums].OriginFunction = &(newHookStub->origin_code);
	g_HookTable[g_HookTableNums].HookLen = hook_len;
	g_HookTableNums++;

	ExReleaseResourceLite(&g_HookLock);

	return newHookId;
}

NTSTATUS LhpHookUninstallHook(HOOKID HookId)
{
	PHOOK_ENTRY HookEntry = NULL;
	ULONG Index = 0;
	DWORD CR0_VALUE = 0;

	
	if (HookId == INVALID_HOOKID)
	{
		return STATUS_INVALID_PARAMETER;
	}

	for (Index = 0; Index < g_HookTableNums; Index++)
	{
		if (g_HookTable[Index].HookId == HookId)
		{
			HookEntry = &g_HookTable[Index];
			break;
		}
	}

	if (HookEntry == NULL || HookEntry->HookStub == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}


	//
	// 关写保护
	//
	_asm
	{  
		//  
		// 关写保护，HOOK时的标准动作
		//        
		cli
		mov   eax, cr0 
		mov   CR0_VALUE, eax
		and   eax, not 10000H 
		mov   cr0, eax   
	}

	memcpy(HookEntry->TargetAddress, HookEntry->HookStub->origin_code, HookEntry->HookLen);
	
	//
	// 开写保护
	//
	_asm
	{
		sti
		mov     eax, CR0_VALUE                  
		mov     cr0, eax                                    
	}

	ExAcquireResourceExclusiveLite(&g_HookLock, TRUE);

	ExFreePool(HookEntry->HookStub);
	RtlZeroMemory(HookEntry, sizeof(HOOK_ENTRY));

	ExReleaseResourceLite(&g_HookLock);

	return STATUS_SUCCESS;
}