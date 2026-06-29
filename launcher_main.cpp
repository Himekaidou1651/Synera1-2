// ============================================================================
// @file    launcher_main.cpp
// @brief   Win32 轻量级启动器，递归查找并启动 Synera.exe
// @author  
// @date    2026-06-24
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <string>

#pragma comment(lib, "shlwapi.lib")

// ============================================================
//  递归查找 Synera.exe（使用 Win32 API，避免 std::filesystem 兼容性问题）
//  返回路径最深的那个（优先 Debug 版）
// ============================================================
std::wstring FindSyneraExe()
{
    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);

    // 从自身所在目录开始
    wchar_t searchDir[MAX_PATH];
    wcscpy_s(searchDir, selfPath);
    PathRemoveFileSpecW(searchDir);

    std::wstring bestMatch;
    int bestDepth = 0;

    // 用递归方式遍历目录
    // 使用栈模拟递归：每个条目为 (目录路径, 当前深度)
    struct DirEntry {
        std::wstring path;
        int depth;
    };

    DirEntry stack[256];
    int stackPos = 0;
    stack[stackPos++] = { searchDir, 0 };

    while (stackPos > 0)
    {
        stackPos--;
        std::wstring dir = stack[stackPos].path;
        int curDepth = stack[stackPos].depth;

        std::wstring searchPattern = dir + L"\\*";

        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                // 跳过 . 和 ..
                if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
                    continue;

                std::wstring fullPath = dir + L"\\" + findData.cFileName;

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // 子目录入栈
                    if (stackPos < 256)
                        stack[stackPos++] = { fullPath, curDepth + 1 };
                }
                else
                {
                    // 检查是否为目标 exe
                    if (_wcsicmp(findData.cFileName, L"Synera.exe") == 0)
                    {
                        // 跳过自身
                        if (_wcsicmp(fullPath.c_str(), selfPath) == 0)
                            continue;

                        if (curDepth > bestDepth)
                        {
                            bestDepth = curDepth;
                            bestMatch = fullPath;
                        }
                    }
                }
            }
            while (FindNextFileW(hFind, &findData));

            FindClose(hFind);
        }
    }

    return bestMatch;
}

// ============================================================
//  显示错误消息框
// ============================================================
void ShowError(const std::wstring &title, const std::wstring &message)
{
    MessageBoxW(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

// ============================================================
//  入口点
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 1. 查找 Synera.exe
    std::wstring exePath = FindSyneraExe();

    if (exePath.empty())
    {
        std::wstring msg =
            L"未找到游戏主程序 Synera.exe。\n\n"
            L"请先运行 run.bat 或 build.ps1 构建项目。\n\n"
            L"构建完成后，此启动器将自动找到并运行游戏。";
        ShowError(L"Synera2026 - 启动失败", msg);
        return 1;
    }

    // 2. 获取 exe 所在目录（作为工作目录）
    wchar_t exeDir[MAX_PATH];
    wcscpy_s(exeDir, exePath.c_str());
    PathRemoveFileSpecW(exeDir);

    // 3. 启动 Synera.exe
    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd         = NULL;
    sei.lpVerb       = L"open";
    sei.lpFile       = exePath.c_str();
    sei.lpParameters = L"";
    sei.lpDirectory  = exeDir;
    sei.nShow        = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei))
    {
        std::wstring msg =
            L"无法启动游戏：\n" + exePath + L"\n\n"
            L"错误代码：" + std::to_wstring(GetLastError());
        ShowError(L"Synera2026 - 启动失败", msg);
        return 1;
    }

    return 0;
}