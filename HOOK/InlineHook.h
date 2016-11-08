#pragma once

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef uint32_t boolean_t;


class CInlineHook
{
public:
	CInlineHook();
	~CInlineHook();

	bool InstallHook(const char* ModuleName, const char* ApiName, void* HandlerAddress);
	bool InstallHook(void* TargetAddress, void* HandlerAddress);
	bool UninstallHook();

	void* m_OriginFunction;


protected:
	//
	// 获取汇编指令长度，用于HOOK
	//
	static int InstructLen(void* Address);
	static const uint8_t c_opinfo[256];


protected:
	typedef unsigned char byte;
	typedef struct _HOOK_STUB
	{
		byte jz[2];
		byte jnz[2];
		byte flag[4];
		byte nop[5];
		byte jmp_handler[5];
		byte origin_code[15];
		byte jmp_origin_fun[5];
		byte flag2[4];
	}HOOK_STUB, *PHOOK_STUB;

	void InitHookStub(PHOOK_STUB HookStub);

	PHOOK_STUB m_HookStub;
	void* m_TargetAddress;
	int m_HookLen;
};

