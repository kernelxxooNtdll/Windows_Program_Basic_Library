
#ifndef __INLINE_HOOK_H__
#define __INLINE_HOOK_H__

#include <ntddk.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef uint32_t boolean_t;
typedef ULONG DWORD;


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


#define HOOK_TABLE_MAX 100
#define INVALID_HOOKID 0
typedef ULONG HOOKID;
typedef struct _HOOK_ENTRY
{
	HOOKID HookId;
	PHOOK_STUB HookStub;
	PVOID TargetAddress;
	PVOID OriginFunction;
	ULONG HookLen;
}HOOK_ENTRY, *PHOOK_ENTRY;


NTSTATUS LhpHookModuleInit();
VOID LhpHookModuleUninit();

int LhpHookInstructLen(void* Address);
HOOKID LhpHookInstallHook(void* TargetAddress, void* HandlerAddress);
NTSTATUS LhpHookUninstallHook(HOOKID HookId);
VOID LhpHookInitHookStub(PHOOK_STUB HookStub);
PVOID LhpHookGetOriginFunAddress(HOOKID HookId);

#endif // __INLINE_HOOK_H__