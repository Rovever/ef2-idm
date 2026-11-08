/* ErrorWatch 回归验证：造两个 #32770 对话框，验证「只关 403 框，不误伤其它框」
 * 纯 Win32，不需要 IDM 运行。
 */
#include <Windows.h>
#include <stdio.h>
#pragma comment(lib, "User32.lib")
#include "../src/errorwatch.h"

static volatile LONG g_target_closed = 0;
static volatile LONG g_ctrl_closed = 0;
static HWND g_target = NULL, g_ctrl = NULL;

static LRESULT CALLBACK TargetProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_COMMAND && LOWORD(w) == IDOK)
    {
        InterlockedExchange(&g_target_closed, 1);
        DestroyWindow(h);
        return 0;
    }
    if (m == WM_CLOSE)
    {
        InterlockedExchange(&g_target_closed, 1);
        DestroyWindow(h);
        return 0;
    }
    if (m == WM_DESTROY) return 0;
    return DefWindowProcW(h, m, w, l);
}
static LRESULT CALLBACK CtrlProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_CLOSE || (m == WM_COMMAND && LOWORD(w) == IDOK))
    {
        InterlockedExchange(&g_ctrl_closed, 1);
        DestroyWindow(h);
        return 0;
    }
    if (m == WM_DESTROY) return 0;
    return DefWindowProcW(h, m, w, l);
}
static DWORD WINAPI WatchThread(LPVOID)
{
    ErrorWatch::Run(8);
    return 0;
}
static HWND MakeDlg(const wchar_t *title, int x, WNDPROC proc, const wchar_t *line1, const wchar_t *line2)
{
    HINSTANCE hi = GetModuleHandleW(NULL);
    HWND h = CreateWindowExW(0, L"#32770", title,
                             WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                             x, 100, 480, 210, NULL, NULL, 0, NULL);
    if (!h) return NULL;
    SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)proc);
    CreateWindowExW(0, L"Static", line1, WS_CHILD | WS_VISIBLE, 20, 25, 430, 25, h, NULL, hi, NULL);
    CreateWindowExW(0, L"Static", line2, WS_CHILD | WS_VISIBLE, 20, 60, 430, 25, h, NULL, hi, NULL);
    CreateWindowExW(0, L"Button", L"确定", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    350, 130, 90, 28, h, (HMENU)IDOK, hi, NULL);
    return h;
}

int main()
{
    g_target = MakeDlg(L"【程潇】《如梦令》跳进了李清照的词.m4a", -2000, TargetProc,
                       L"服务器响应显示您没有权限下载此文件。",
                       L"备注: \\nHTTP/1.1 403 Forbidden");
    g_ctrl = MakeDlg(L"下载文件信息", -1500, CtrlProc,
                     L"文件名: normal.zip",
                     L"保存到: D:\\Downloads");
    if (!g_target || !g_ctrl)
    {
        printf("create test dialogs failed, err=%lu\n", GetLastError());
        return 1;
    }

    HANDLE th = CreateThread(NULL, 0, WatchThread, NULL, 0, NULL);
    DWORD start = GetTickCount();
    MSG msg;
    while (GetTickCount() - start < 6000 && !g_target_closed)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(20);
    }
    printf("403 error dialog auto-closed : %s (%lu ms)\n",
           g_target_closed ? "YES" : "NO", GetTickCount() - start);
    printf("normal download dialog closed: %s (should be NO)\n",
           (g_ctrl_closed || !IsWindow(g_ctrl)) ? "YES" : "NO");
    WaitForSingleObject(th, 9000);
    CloseHandle(th);
    return (g_target_closed && !g_ctrl_closed && IsWindow(g_ctrl)) ? 0 : 2;
}
