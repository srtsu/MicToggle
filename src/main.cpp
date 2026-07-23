#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "audio_endpoint.h"
#include "wav_resource.h"

static const UINT kHotkeyVk = VK_LAUNCH_MEDIA_SELECT;
static const UINT WM_HOTKEY_ACTION = WM_APP + 1;

static DWORD g_mainThreadId = 0;
static HHOOK g_hook = nullptr;
static volatile LONG g_actionPending = 0;
static volatile LONG g_hookKeyDown = 0;

static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        const auto* kb = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (kb->vkCode == kHotkeyVk)
        {
            if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                InterlockedExchange(&g_hookKeyDown, 0);
                return 1;
            }

            if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) &&
                InterlockedExchange(&g_hookKeyDown, 1) == 0 &&
                InterlockedExchange(&g_actionPending, 1) == 0)
            {
                if (!PostThreadMessageW(g_mainThreadId, WM_HOTKEY_ACTION, 0, 0))
                    InterlockedExchange(&g_actionPending, 0);
            }
            return 1;
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

static bool TryRegisterMediaHotkey()
{
    return RegisterHotKey(nullptr, 1, MOD_NOREPEAT, kHotkeyVk) != 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int)
{
    g_mainThreadId = GetCurrentThreadId();

    MSG tmp;
    PeekMessageW(&tmp, nullptr, 0, 0, PM_NOREMOVE);

    MicEndpoint mic;
    ResourceSounds sounds;
    if (!sounds.Init(hInst)) return 1;

    if (!mic.Init())
    {
        sounds.PlayError();
        return 2;
    }

    const bool hotkeyOk = TryRegisterMediaHotkey();
    if (!hotkeyOk)
    {
        g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInst, 0);
        if (!g_hook) return static_cast<int>(GetLastError());
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        const bool isAction =
            (hotkeyOk && msg.message == WM_HOTKEY && msg.wParam == 1) ||
            (!hotkeyOk && msg.message == WM_HOTKEY_ACTION);

        if (!isAction) continue;

        if (hotkeyOk && msg.message == WM_HOTKEY)
        {
            if (InterlockedExchange(&g_actionPending, 1) != 0) continue;
        }

        bool newMuted = false;
        if (mic.ToggleMute(newMuted))
        {
            newMuted ? sounds.PlayMuted() : sounds.PlayUnmuted();
        }
        else
        {
            bool curMuted = false;
            if (mic.GetMute(curMuted))
                curMuted ? sounds.PlayMuted() : sounds.PlayUnmuted();
            else
                sounds.PlayError();
        }

        InterlockedExchange(&g_actionPending, 0);
    }

    if (hotkeyOk) UnregisterHotKey(nullptr, 1);
    if (g_hook) UnhookWindowsHookEx(g_hook);

    mic.Shutdown();
    return 0;
}
