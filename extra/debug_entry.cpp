#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1

#include <chrono>
#include <codecvt>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "../app.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winternl.h>
#include <psapi.h>
#include <shellapi.h>
#pragma comment(lib, "ntdll.lib")

#else
#ifdef __linux__

// TODO: Linux implementation
#error "Not implemented"

#else

// TODO: MacOS implementation
#error "Not implemented"

#endif
#endif

std::wstring plugin_uuid = L"com.alexguo1998.flexbarplugin-qt";

#include <locale>
#include <locale.h>

#ifndef MS_STDLIB_BUGS
#  if ( _MSC_VER || __MINGW32__ || __MSVCRT__ )
#    define MS_STDLIB_BUGS 1
#  else
#    define MS_STDLIB_BUGS 0
#  endif
#endif

#if MS_STDLIB_BUGS
#  include <io.h>
#  include <fcntl.h>
#endif

void init_locale(void) {
#if MS_STDLIB_BUGS
    constexpr char cp_utf16le[] = ".1200";
    setlocale(LC_ALL, cp_utf16le);
    _setmode(_fileno(stdout), _O_WTEXT);
#else
    // The correct locale name may vary by OS, e.g., "en_US.utf8".
    constexpr char locale_name[] = "";
    setlocale(LC_ALL, locale_name);
    std::locale::global(std::locale(locale_name));
    std::wcin.imbue(std::locale())
        std::wcout.imbue(std::locale());
#endif
}

struct HandleCloser {
    HandleCloser(HANDLE h) :h_(h) {}
    ~HandleCloser() { CloseHandle(h_); }
    HANDLE h_;
};

bool find_and_kill_extension_process(std::vector<std::string>& argv) {
    DWORD cbNeeded, cb;
    DWORD count = 1024 / 2;
    std::vector<DWORD> pids;
    do {
        count *= 2;
        pids.resize(count);
        cb = count * sizeof(DWORD);
        if (!EnumProcesses(&pids.front(), cb, &cbNeeded)) {
            return false;
        }
    } while (cbNeeded >= cb);
    pids.resize(cbNeeded / sizeof(DWORD));

    std::wstring expected_arg = L" --uid=" + plugin_uuid + L" ";
    bool found = false;

    for (DWORD pid : pids) {
        HANDLE hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == nullptr) continue;
        HandleCloser closer(hProcess);

        std::wstring name(1024, L'\0');
        DWORD name_len = GetModuleBaseNameW(hProcess, nullptr, &name.front(), name.size());
        name.resize(name_len);
        if (name != L"FlexDesigner.exe") continue;

        PROCESS_BASIC_INFORMATION pbi;
        LONG status = NtQueryInformationProcess(
            hProcess, ProcessBasicInformation,
            &pbi, sizeof(pbi),
            nullptr);
        if (!NT_SUCCESS(status)) continue;

        PEB peb;
        BOOL result = ReadProcessMemory(
            hProcess, pbi.PebBaseAddress,
            &peb, sizeof(peb),
            nullptr);
        if (!result) continue;

        RTL_USER_PROCESS_PARAMETERS rtlProcParam;
        result = ReadProcessMemory(
            hProcess, peb.ProcessParameters,
            &rtlProcParam, sizeof(rtlProcParam),
            nullptr);
        if (!result) continue;

        USHORT len = rtlProcParam.CommandLine.Length;
        std::wstring commandLine(len, L'\0');
        result = ReadProcessMemory(
            hProcess, rtlProcParam.CommandLine.Buffer,
            &commandLine.front(), len,
            nullptr);
        if (!result) continue;

        // std::wcout << L"Scanning: " << name << L" (" << pid << L")\nCMD: " << commandLine << L"\n\n";

        if (commandLine.find(expected_arg) != std::wstring::npos) {
            int count;
            LPWSTR* argv_c = CommandLineToArgvW(commandLine.c_str(), &count);

            static_assert(sizeof(wchar_t) == sizeof(char16_t), "use codecvt_utf8 for wchar_t == char32_t");
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
            argv.clear();
            for (int i = 0; i < count; ++i) {
                argv.push_back(conv.to_bytes(argv_c[i]));
            }

            LocalFree(argv_c);
            TerminateProcess(hProcess, 0);
            return true;
        }
    }

    return false;
}


int main() {
    init_locale();

    std::vector<std::string> args;
    if (!find_and_kill_extension_process(args)) {
        std::wcout << L"No existing plugin process found, using flexcli to restart the plugin...\n";
        std::wstring cmd = L"flexcli plugin restart --uuid " + plugin_uuid;
        int code = _wsystem(cmd.c_str());
        if (code) {
            std::wcout <<
                L"flexcli invocation failed. Args:\n" << cmd << L"\n"
                L"Please check:\n"
                "* Is the plugin registered with\n"
                "    `flexcli plugin link --path <path> --uuid " << plugin_uuid << L" --force`?\n"
                "* Is FlexDesigner running?\n";
            return 1;
        }
        for (int i = 0; i < 100; ++i) {
            if (find_and_kill_extension_process(args)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (args.empty()) {
            std::wcout <<
                L"Still no plugin process found. Please check:\n"
                "* Is the plugin crashing immediately after launch?\n";
            return 1;
        }
    }

    std::wcout << L"Starting now...\n";
    return qt_main(args);
}
