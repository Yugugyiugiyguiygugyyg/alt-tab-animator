// ==WindhawkMod==
// @id              alt-tab-animation
// @name            Alt+Tab Smooth Animation
// @description     Smooth customizable entrance and exit animations for the Windows 11 Alt+Tab task switcher.
// @version         2.0.0
// @author          Community
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -ldwmapi -luser32 -lgdi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Alt+Tab Smooth Animation for Windows 11

Adds smooth, customizable entrance AND exit transition animations to the Windows 11 Alt+Tab switcher.

### Features:
- **Smooth Entrance Animations:** Choose from 14 customizable styles (Slide Up/Down/Left/Right, Zoom In/Out, Pop, Bounce, Fade).
- **Smooth Exit Animations (Ghost Snapshot Cross-Fade):** Overcomes Windows 11's XAML unrendering by capturing a lightweight hardware snapshot on dismiss and smoothly fading it out into the newly focused window.
- **Zero Input Lag:** Windows immediately switches focus to your target application upon releasing Alt. The exit overlay is completely click-through (`WS_EX_TRANSPARENT`).
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- entranceStyle: slideUpFade
  $name: Entrance animation style
  $description: Choose from 14 distinct entrance styles.
  $options:
  - slideUpFade: Fade + Slide Up (Recommended)
  - popZoom: Pop & Overshoot (Bouncy Zoom)
  - bounceUp: Bounce & Overshoot (Bouncy Slide Up)
  - zoomInFade: Fade + Zoom In (Scale Up)
  - zoomOutFade: Fade + Zoom Out (Drop Down)
  - slideDownFade: Fade + Slide Down
  - slideLeftFade: Fade + Slide from Right
  - slideRightFade: Fade + Slide from Left
  - fadeOnly: Fade Only (No Movement)
  - slideUpSolid: Solid Slide Up (No Fade)
  - slideDownSolid: Solid Slide Down (No Fade)
  - slideLeftSolid: Solid Slide from Right (No Fade)
  - slideRightSolid: Solid Slide from Left (No Fade)
  - zoomSolid: Solid Zoom In (No Fade)

- exitFade: true
  $name: Enable smooth exit animation
  $description: Smoothly dissolves the Alt+Tab menu on dismiss instead of instantly blinking out.

- duration: 150
  $name: Entrance duration (ms)
  $description: Duration of the entrance animation in milliseconds (60-350 ms).

- exitDuration: 180
  $name: Exit fade duration (ms)
  $description: Duration of the exit fade in milliseconds (80-350 ms).

- slideDistance: 24
  $name: Slide distance (px)
  $description: Movement distance in pixels for slide animations.

- zoomScale: 92
  $name: Zoom start scale (%)
  $description: Scale factor for Zoom In animations (70-98%).
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <windhawk_api.h>
#include <atomic>
#include <thread>
#include <chrono>

enum class AnimStyle {
    SlideUpFade,
    SlideDownFade,
    SlideLeftFade,
    SlideRightFade,
    ZoomInFade,
    ZoomOutFade,
    PopZoom,
    BounceUp,
    FadeOnly,
    SlideUpSolid,
    SlideDownSolid,
    SlideLeftSolid,
    SlideRightSolid,
    ZoomSolid
};

struct Settings {
    AnimStyle entranceStyle;
    bool      exitFade;
    int       duration;
    int       exitDuration;
    int       slideDistance;
    int       zoomScale;
} g_settings;

AnimStyle ParseStyle(PCWSTR str) {
    if (!str) return AnimStyle::SlideUpFade;
    if (wcscmp(str, L"slideUpFade") == 0) return AnimStyle::SlideUpFade;
    if (wcscmp(str, L"popZoom") == 0) return AnimStyle::PopZoom;
    if (wcscmp(str, L"bounceUp") == 0) return AnimStyle::BounceUp;
    if (wcscmp(str, L"zoomInFade") == 0) return AnimStyle::ZoomInFade;
    if (wcscmp(str, L"zoomOutFade") == 0) return AnimStyle::ZoomOutFade;
    if (wcscmp(str, L"slideDownFade") == 0) return AnimStyle::SlideDownFade;
    if (wcscmp(str, L"slideLeftFade") == 0) return AnimStyle::SlideLeftFade;
    if (wcscmp(str, L"slideRightFade") == 0) return AnimStyle::SlideRightFade;
    if (wcscmp(str, L"fadeOnly") == 0) return AnimStyle::FadeOnly;
    if (wcscmp(str, L"slideUpSolid") == 0) return AnimStyle::SlideUpSolid;
    if (wcscmp(str, L"slideDownSolid") == 0) return AnimStyle::SlideDownSolid;
    if (wcscmp(str, L"slideLeftSolid") == 0) return AnimStyle::SlideLeftSolid;
    if (wcscmp(str, L"slideRightSolid") == 0) return AnimStyle::SlideRightSolid;
    if (wcscmp(str, L"zoomSolid") == 0) return AnimStyle::ZoomSolid;
    return AnimStyle::SlideUpFade;
}

void LoadSettings() {
    PCWSTR str = Wh_GetStringSetting(L"entranceStyle");
    g_settings.entranceStyle = ParseStyle(str);
    if (str) Wh_FreeStringSetting(str);

    g_settings.exitFade = Wh_GetIntSetting(L"exitFade") != 0;

    g_settings.duration = Wh_GetIntSetting(L"duration");
    if (g_settings.duration < 40) g_settings.duration = 40;
    if (g_settings.duration > 500) g_settings.duration = 500;

    g_settings.exitDuration = Wh_GetIntSetting(L"exitDuration");
    if (g_settings.exitDuration < 40) g_settings.exitDuration = 40;
    if (g_settings.exitDuration > 500) g_settings.exitDuration = 500;

    g_settings.slideDistance = Wh_GetIntSetting(L"slideDistance");
    if (g_settings.slideDistance < 4) g_settings.slideDistance = 4;
    if (g_settings.slideDistance > 160) g_settings.slideDistance = 160;

    g_settings.zoomScale = Wh_GetIntSetting(L"zoomScale");
    if (g_settings.zoomScale < 70) g_settings.zoomScale = 70;
    if (g_settings.zoomScale > 98) g_settings.zoomScale = 98;
}

static std::atomic<uint64_t> g_animGen(0);
static HWND                  g_hGhostWnd = nullptr;

using ShowWindow_t = BOOL(WINAPI*)(HWND hWnd, int nCmdShow);
ShowWindow_t pOriginalShowWindow = nullptr;

inline float EaseOutCubic(float t) {
    float inv = 1.0f - t;
    return 1.0f - (inv * inv * inv);
}

inline float EaseOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float inv = t - 1.0f;
    return 1.0f + c3 * (inv * inv * inv) + c1 * (inv * inv);
}

bool IsAltTabWindow(HWND hWnd) {
    if (!hWnd || !IsWindow(hWnd)) return false;

    wchar_t className[128] = {0};
    if (GetClassNameW(hWnd, className, ARRAYSIZE(className)) == 0) return false;

    return (wcscmp(className, L"XamlExplorerHostIslandWindow") == 0 ||
            wcscmp(className, L"MultitaskingViewFrame") == 0);
}

inline bool HasFade(AnimStyle style) {
    switch (style) {
        case AnimStyle::SlideUpSolid:
        case AnimStyle::SlideDownSolid:
        case AnimStyle::SlideLeftSolid:
        case AnimStyle::SlideRightSolid:
        case AnimStyle::ZoomSolid:
            return false;
        default:
            return true;
    }
}

inline bool HasOvershoot(AnimStyle style) {
    return (style == AnimStyle::PopZoom || style == AnimStyle::BounceUp);
}

// -------------------------------------------------------------
// Entrance Animation
// -------------------------------------------------------------
void StartEntranceAnimation(HWND hWnd, const Settings& s) {
    uint64_t currentGen = ++g_animGen;

    // If ghost exit overlay is currently visible, hide it immediately
    if (g_hGhostWnd && IsWindow(g_hGhostWnd)) {
        ShowWindow(g_hGhostWnd, SW_HIDE);
    }

    LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_LAYERED)) {
        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
    }
    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, (exStyle | WS_EX_LAYERED) & ~WS_EX_TRANSPARENT);

    bool useFade = HasFade(s.entranceStyle);
    bool useOvershoot = HasOvershoot(s.entranceStyle);

    SetLayeredWindowAttributes(hWnd, 0, useFade ? 0 : 255, LWA_ALPHA);

    RECT origRect = {0};
    GetWindowRect(hWnd, &origRect);
    int origW = origRect.right - origRect.left;
    int origH = origRect.bottom - origRect.top;
    int origX = origRect.left;
    int origY = origRect.top;

    if (origW <= 0) origW = GetSystemMetrics(SM_CXSCREEN);
    if (origH <= 0) origH = GetSystemMetrics(SM_CYSCREEN);

    float minScale = (float)s.zoomScale / 100.0f;
    int dist = s.slideDistance;
    AnimStyle style = s.entranceStyle;

    int startX = origX;
    int startY = origY;
    int startW = origW;
    int startH = origH;

    switch (style) {
        case AnimStyle::SlideUpFade:
        case AnimStyle::SlideUpSolid:
        case AnimStyle::BounceUp:
            startY = origY + dist;
            break;

        case AnimStyle::SlideDownFade:
        case AnimStyle::SlideDownSolid:
            startY = origY - dist;
            break;

        case AnimStyle::SlideLeftFade:
        case AnimStyle::SlideLeftSolid:
            startX = origX + dist;
            break;

        case AnimStyle::SlideRightFade:
        case AnimStyle::SlideRightSolid:
            startX = origX - dist;
            break;

        case AnimStyle::ZoomInFade:
        case AnimStyle::ZoomSolid:
        case AnimStyle::PopZoom:
            startW = (int)(origW * minScale + 0.5f);
            startH = (int)(origH * minScale + 0.5f);
            startX = origX + (origW - startW) / 2;
            startY = origY + (origH - startH) / 2;
            break;

        case AnimStyle::ZoomOutFade:
            startW = (int)(origW * 1.08f + 0.5f);
            startH = (int)(origH * 1.08f + 0.5f);
            startX = origX + (origW - startW) / 2;
            startY = origY + (origH - startH) / 2;
            break;

        default:
            break;
    }

    SetWindowPos(hWnd, NULL, startX, startY, startW, startH, SWP_NOZORDER | SWP_NOACTIVATE);
    pOriginalShowWindow(hWnd, SW_SHOWNA);

    int duration = s.duration;
    std::thread([hWnd, duration, currentGen, origX, origY, origW, origH, dist, minScale, style, useFade, useOvershoot]() {
        const int steps = 16;
        int stepDelay = duration / steps;
        if (stepDelay < 5) stepDelay = 5;

        for (int i = 1; i <= steps; i++) {
            if (g_animGen.load() != currentGen) return;

            float progress = (float)i / (float)steps;
            float eased = useOvershoot ? EaseOutBack(progress) : EaseOutCubic(progress);

            if (useFade) {
                float fadeEased = EaseOutCubic(progress);
                int alpha = (int)(255.0f * fadeEased + 0.5f);
                if (alpha > 255) alpha = 255;
                SetLayeredWindowAttributes(hWnd, 0, (BYTE)alpha, LWA_ALPHA);
            }

            int curX = origX;
            int curY = origY;
            int curW = origW;
            int curH = origH;

            switch (style) {
                case AnimStyle::SlideUpFade:
                case AnimStyle::SlideUpSolid:
                case AnimStyle::BounceUp:
                    curY = origY + (int)(dist * (1.0f - eased) + 0.5f);
                    break;

                case AnimStyle::SlideDownFade:
                case AnimStyle::SlideDownSolid:
                    curY = origY - (int)(dist * (1.0f - eased) + 0.5f);
                    break;

                case AnimStyle::SlideLeftFade:
                case AnimStyle::SlideLeftSolid:
                    curX = origX + (int)(dist * (1.0f - eased) + 0.5f);
                    break;

                case AnimStyle::SlideRightFade:
                case AnimStyle::SlideRightSolid:
                    curX = origX - (int)(dist * (1.0f - eased) + 0.5f);
                    break;

                case AnimStyle::ZoomInFade:
                case AnimStyle::ZoomSolid:
                case AnimStyle::PopZoom: {
                    float curScale = minScale + (1.0f - minScale) * eased;
                    curW = (int)(origW * curScale + 0.5f);
                    curH = (int)(origH * curScale + 0.5f);
                    curX = origX + (origW - curW) / 2;
                    curY = origY + (origH - curH) / 2;
                    break;
                }

                case AnimStyle::ZoomOutFade: {
                    float curScale = 1.08f - 0.08f * eased;
                    curW = (int)(origW * curScale + 0.5f);
                    curH = (int)(origH * curScale + 0.5f);
                    curX = origX + (origW - curW) / 2;
                    curY = origY + (origH - curH) / 2;
                    break;
                }

                case AnimStyle::FadeOnly:
                default:
                    break;
            }

            if (style != AnimStyle::FadeOnly) {
                SetWindowPos(hWnd, NULL, curX, curY, curW, curH, SWP_NOZORDER | SWP_NOACTIVATE);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(stepDelay));
        }

        if (g_animGen.load() == currentGen) {
            SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
            SetWindowPos(hWnd, NULL, origX, origY, origW, origH, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }).detach();
}

// -------------------------------------------------------------
// Exit Animation (Ghost Overlay Snapshot Cross-Fade)
// -------------------------------------------------------------
void StartExitFade(HWND hWnd, const Settings& s) {
    if (!s.exitFade) {
        pOriginalShowWindow(hWnd, SW_HIDE);
        return;
    }

    uint64_t currentGen = ++g_animGen;

    // Get virtual screen dimensions to capture the exact displayed frame
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) {
        pOriginalShowWindow(hWnd, SW_HIDE);
        return;
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, vw, vh);
    if (!hdcMem || !hBmp) {
        if (hBmp) DeleteObject(hBmp);
        if (hdcMem) DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        pOriginalShowWindow(hWnd, SW_HIDE);
        return;
    }

    HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

    // Instant snapshot: captures the exact frame showing Alt+Tab before unrender
    BitBlt(hdcMem, 0, 0, vw, vh, hdcScreen, vx, vy, SRCCOPY);

    // Immediately hide the real window: target window activates in 0ms (ZERO input lag!)
    pOriginalShowWindow(hWnd, SW_HIDE);

    // Create persistent ghost window if needed
    if (!g_hGhostWnd || !IsWindow(g_hGhostWnd)) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"WindhawkAltTabGhost";
        RegisterClassExW(&wc);

        g_hGhostWnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            L"WindhawkAltTabGhost",
            L"",
            WS_POPUP,
            vx, vy, vw, vh,
            NULL, NULL, GetModuleHandleW(NULL), NULL
        );
    }

    HWND hGhost = g_hGhostWnd;
    if (!hGhost) {
        SelectObject(hdcMem, hOld);
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    POINT ptDst = { vx, vy };
    POINT ptSrc = { 0, 0 };
    SIZE sz = { vw, vh };
    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;

    // Display the snapshot with 100% opacity initially (seamless handoff)
    UpdateLayeredWindow(hGhost, hdcScreen, &ptDst, &sz, hdcMem, &ptSrc, 0, &bf, ULW_ALPHA);
    ShowWindow(hGhost, SW_SHOWNOACTIVATE);

    int duration = s.exitDuration;

    // Asynchronously fade the snapshot out into the newly focused window
    std::thread([hGhost, duration, currentGen, hdcMem, hBmp, hOld, hdcScreen]() {
        const int steps = 15;
        int stepDelay = duration / steps;
        if (stepDelay < 6) stepDelay = 6;

        for (int i = 1; i <= steps; i++) {
            if (g_animGen.load() != currentGen) break;

            float progress = (float)i / (float)steps;
            float eased = EaseOutCubic(progress);
            int alpha = (int)(255.0f * (1.0f - eased) + 0.5f);
            if (alpha < 0) alpha = 0;

            BLENDFUNCTION blend = {};
            blend.BlendOp = AC_SRC_OVER;
            blend.SourceConstantAlpha = (BYTE)alpha;

            UpdateLayeredWindow(hGhost, NULL, NULL, NULL, NULL, NULL, 0, &blend, ULW_ALPHA);
            std::this_thread::sleep_for(std::chrono::milliseconds(stepDelay));
        }

        if (g_animGen.load() == currentGen) {
            ShowWindow(hGhost, SW_HIDE);
        }

        // Clean up GDI snapshot resources
        SelectObject(hdcMem, hOld);
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
    }).detach();
}

// -------------------------------------------------------------
// ShowWindow Hook
// -------------------------------------------------------------
BOOL WINAPI ShowWindow_Hook(HWND hWnd, int nCmdShow) {
    if (IsAltTabWindow(hWnd)) {
        if (nCmdShow == SW_SHOWNA || nCmdShow == SW_SHOW) {
            StartEntranceAnimation(hWnd, g_settings);
            return TRUE;
        } else if (nCmdShow == SW_HIDE) {
            StartExitFade(hWnd, g_settings);
            return TRUE;
        }
    }
    return pOriginalShowWindow(hWnd, nCmdShow);
}

BOOL Wh_ModInit() {
    Wh_Log(L"[AltTab Animation v2.0.0] Initializing entrance + exit fade...");
    LoadSettings();

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) hUser32 = LoadLibraryW(L"user32.dll");
    if (!hUser32) return FALSE;

    void* pShowWindow = (void*)GetProcAddress(hUser32, "ShowWindow");
    if (!pShowWindow) return FALSE;

    if (!Wh_SetFunctionHook(pShowWindow, (void*)ShowWindow_Hook, (void**)&pOriginalShowWindow)) {
        Wh_Log(L"[AltTab Animation] Failed to hook ShowWindow.");
        return FALSE;
    }

    Wh_Log(L"[AltTab Animation v2.0.0] Active with smooth entrance & exit animations!");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"[AltTab Animation v2.0.0] Unloading mod.");
    ++g_animGen;
    if (g_hGhostWnd && IsWindow(g_hGhostWnd)) {
        DestroyWindow(g_hGhostWnd);
        g_hGhostWnd = nullptr;
    }
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}
