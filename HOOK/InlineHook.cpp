#include "stdafx.h"
#include "InlineHook.h"

#include <stdio.h>

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

const uint8_t CInlineHook::c_opinfo[256] = {
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

int CInlineHook::InstructLen(void* Address)
{
	enum cpu_mode_t cm = cm_compat;
	register uint8_t* p = (uint8_t*)Address;
	register uint8_t b = 0;
	register boolean_t pre_66 = false;
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
			pre_66 = true;
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
			boolean_t has_sib = false;
			register uint8_t tmp = 0;

			if ((modrm & 0xc0) == 0xc0)	/* mod == 3 */
				break;

			if (cm != cm_legacy && (modrm & 0x07) == 4) {
				has_sib = true;
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



CInlineHook::CInlineHook()
{
	m_HookStub = NULL;
	m_OriginFunction = NULL;
	m_TargetAddress = NULL;
	m_HookLen = 0;
}


CInlineHook::~CInlineHook()
{
	if (m_HookStub)
	{
		delete m_HookStub;
		m_HookStub = NULL;
	}
}

void CInlineHook::InitHookStub(PHOOK_STUB HookStub)
{
	if (HookStub == NULL)
	{
		return;
	}

	ZeroMemory(HookStub, sizeof(HOOK_STUB));
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

bool CInlineHook::InstallHook(const char* ModuleName, const char* ApiName, void* HandlerAddress)
{
	HMODULE hModule = NULL;
	void* targetAddress = NULL;
	bool bRet = false;

	do 
	{
		hModule = GetModuleHandleA(ModuleName);
		if (!hModule)
			break;

		targetAddress = GetProcAddress(hModule, ApiName);
		if (!targetAddress)
			break;

		bRet = InstallHook(targetAddress, HandlerAddress);

	} while (false);

	return bRet;
}

bool CInlineHook::InstallHook(void* TargetAddress, void* HandlerAddress)
{
	DWORD OldPageProperty = 0;

	if (TargetAddress == NULL || HandlerAddress == NULL || m_HookStub)
	{
		return false;
	}

	//
	// 计算要HOOK的字节长度
	//
	int hook_len = 0;
	uint8_t* start_address = (uint8_t*)TargetAddress;
	while (hook_len < 5)
	{
		int instruct_len = InstructLen(start_address);
		if (instruct_len <= 0 || instruct_len >= 10)
			break;

		hook_len += instruct_len;
		start_address += instruct_len;
	}

	WCHAR buf[100] = { 0 };
	swprintf(buf, L"Hook len: %d\n", hook_len);
	OutputDebugStringW(buf);

	//
	// 分配中间跳指令区，并设置可执行属性
	//
	m_HookStub = new HOOK_STUB();
	if (m_HookStub == NULL)
	{
		return false;
	}
	if (!VirtualProtect(m_HookStub, sizeof(HOOK_STUB), PAGE_EXECUTE_READWRITE, &OldPageProperty))
	{
		delete m_HookStub;
		m_HookStub = NULL;
	}

	//
	// 准备hook中间跳转指令
	//
	InitHookStub(m_HookStub);
	memcpy(m_HookStub->origin_code, TargetAddress, hook_len);
	*((unsigned long *)(&m_HookStub->jmp_handler[1])) = (unsigned long)HandlerAddress - ((unsigned long)&m_HookStub->jmp_handler + 5);
	*((unsigned long *)(&m_HookStub->jmp_origin_fun[1])) = ((unsigned long)TargetAddress + hook_len) - ((unsigned long)&m_HookStub->jmp_origin_fun + 5);


	//
	// 准备Hook目标入口跳转指令
	//
	byte jmp_entry[20] = { 0 };
	memset(jmp_entry, 0x90, sizeof(jmp_entry));
	jmp_entry[0] = 0xE9;
	*((unsigned long*)(&jmp_entry[1])) = (unsigned long)m_HookStub - ((unsigned long)TargetAddress + 5);

	//
	// 开始安装钩子
	//
	OldPageProperty = 0;
	VirtualProtect(TargetAddress, hook_len, PAGE_READWRITE, &OldPageProperty);
	memcpy(TargetAddress, jmp_entry, hook_len);
	VirtualProtect(TargetAddress, hook_len, OldPageProperty, &OldPageProperty);

	m_OriginFunction = (void*)(&m_HookStub->origin_code);
	m_TargetAddress = TargetAddress;
	m_HookLen = hook_len;
	return true;
}

bool CInlineHook::UninstallHook()
{
	if (m_HookStub == NULL)
	{
		return false;
	}

	DWORD OldPageProperty = 0;

	VirtualProtect(m_TargetAddress, m_HookLen, PAGE_READWRITE, &OldPageProperty);
	memcpy(m_TargetAddress, m_HookStub->origin_code, m_HookLen);
	VirtualProtect(m_TargetAddress, m_HookLen, OldPageProperty, &OldPageProperty);

	delete m_HookStub;
	m_HookStub = NULL;

	return false;
}