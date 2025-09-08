#include "Log.h"
#include <process.h>
#include <iostream>
#include <TlHelp32.h>

static int g_LogLevel;
static ELogMode g_LogMode;

static FILE* g_LogFile;
static CRingBuffer g_LogMsgQ(40000);
static SRWLOCK g_LogMsgQLock;

thread_local wchar_t gt_LogBuf[LOG_BUFFER_LEN];

static HANDLE g_MsgEvent;

// 버퍼에 이어붙이기
static inline void wappend(wchar_t* dst, size_t cap, int& off, const wchar_t* fmt, ...)
{
	if (off >= (int)cap)
		return;
	va_list ap; va_start(ap, fmt);
	int n = _vsnwprintf_s(dst + off, cap - off, _TRUNCATE, fmt, ap);
	va_end(ap);
	if (n > 0) off += n;
}

// 한 줄 만들기 + 쓰기
static inline void WriteOneLine(CLogMessage& m)
{
	wchar_t line[2048];
	int off = 0;

	// 헤더: [tid] / seq:.. / msgName /
	wappend(line, _countof(line), off, L"[Tid:%u] / seq:%u / %hs / ", m.GetTID(), m.seq, m.msgName);

	// 인자들
	int n = (m.argNum >= 0 && m.argNum <= 5) ? m.argNum : 0;
	for (int i = 0; i < n; ++i)
	{
		const st_ArgEntry& argEntry = m.argEntries[i];
		const char* name = argEntry.argName;
		switch (argEntry.type)
		{
		case I32:
			wappend(line, _countof(line), off, L"%hs:%d / ", name, argEntry.i32);
			break;
		case U32:
			wappend(line, _countof(line), off, L"%hs:%u / ", name, argEntry.u32);
			break;
		case I64:
			wappend(line, _countof(line), off, L"%hs:%lld / ", name, (long long)argEntry.i64);
			break;
		case U64:
			wappend(line, _countof(line), off, L"%hs:%llu / ", name, (unsigned long long)argEntry.u64);
			break;
		case DBL:
			wappend(line, _countof(line), off, L"%hs:%.17g / ", name, argEntry.dbl);
			break;
		case PTR:
			wappend(line, _countof(line), off, L"%hs:%016llX / ",name, (unsigned long long)(uintptr_t)argEntry.ptr);
			break;
		case STR:
			wappend(line, _countof(line), off, L"%hs:%hs / ", name, argEntry.str ? argEntry.str : "");
			break;
		case B:
			wappend(line, _countof(line), off, L"%hs:%hs / ", name, argEntry.b ? L"true" : L"false");
			break;
		case C:
			wappend(line, _countof(line), off, L"%hs:%c / ", name, argEntry.ch);
			break;
		default:
			wappend(line, _countof(line), off, L"%hs:? / ", name);
			break;
		}
	}
	wappend(line, _countof(line), off, L"\n");
	fputws(line, g_LogFile);
}

unsigned int LogThreadProc(void* arg)
{
	while (1)
	{
		if (g_LogMsgQ.GetUseSize() == 0)
			WaitForSingleObject(g_MsgEvent, INFINITE);

		// 작업 메시지 언 마샬링 후 파일 쓰기
		for(;;)
		{
			if (g_LogMsgQ.GetUseSize() < sizeof(CLogMessage))
				break;
			CLogMessage msg;
			g_LogMsgQ.Dequeue((char*)&msg, sizeof(CLogMessage));

			WriteOneLine(msg);
		}
	}

	fflush(g_LogFile);
	fclose(g_LogFile);

	return 0;
}


bool InitLog(int logLevel, ELogMode logMode)
{
	g_LogLevel = logLevel;
	g_LogMode = logMode;

	if (logMode == ELogMode::FILE_DIRECT)
	{
		//---------------------------------------------------------
		// 로그 파일 제목 설정
		//---------------------------------------------------------
		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);
		char logTitle[70];
		sprintf_s(logTitle, sizeof(logTitle), "Log_%04u-%02u-%02u_%02u-%02u-%02u.txt", systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

		errno_t ret;
		ret = fopen_s(&g_LogFile, (const char*)logTitle, "wt");
		if (ret != 0)
			return false;
	}
	else if(logMode == ELogMode::FILE_THREAD)
	{
		InitializeSRWLock(&g_LogMsgQLock);
		g_MsgEvent = CreateEvent(nullptr, false, false, nullptr);
		
		//---------------------------------------------------------
		// 로그 파일 제목 설정 및 파일 열기
		//---------------------------------------------------------
		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);
		char logTitle[70];
		sprintf_s(logTitle, sizeof(logTitle), "Log_%04u-%02u-%02u_%02u-%02u-%02u.txt", systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

		errno_t ret;
		ret = fopen_s(&g_LogFile, (const char*)logTitle, "wt");
		if (ret != 0)
			return false;

		//---------------------------------------------------------
		// 파일 쓰기를 수행할 스레드 생성
		//---------------------------------------------------------
		HANDLE logThread = (HANDLE)_beginthreadex(nullptr, 0, LogThreadProc, nullptr, 0, nullptr);
	}

	return true;
}


void Log(int level, const wchar_t* fmt, ...)
{
	if (level < g_LogLevel)
		return;

	DWORD tid = GetCurrentThreadId();
	int prefixLen = swprintf_s(gt_LogBuf, LOG_BUFFER_LEN, L"[TID:%u] ", tid);
	if (prefixLen < 0)
		prefixLen = 0;

	va_list ap;
	va_start(ap, fmt);
	HRESULT hr = StringCchVPrintfW(gt_LogBuf + prefixLen, LOG_BUFFER_LEN - prefixLen, fmt, ap);
	va_end(ap);

	if (g_LogMode == ELogMode::CONSOLE)
	{
		wprintf(gt_LogBuf);
	}
	else if(g_LogMode == ELogMode::FILE_DIRECT)
	{
		fputws(gt_LogBuf, g_LogFile);
		fflush(g_LogFile);
	}
	
}

void Log(int level, CLogMessage* m)
{
	if (level < g_LogLevel)
		return;

	AcquireSRWLockExclusive(&g_LogMsgQLock);
	int enqueueRet = g_LogMsgQ.Enqueue((char*)m, sizeof(CLogMessage));
	ReleaseSRWLockExclusive(&g_LogMsgQLock);
	if (enqueueRet == 0)
		wprintf(L"LogMsgQ is Full!\n");

	SetEvent(g_MsgEvent);
}

