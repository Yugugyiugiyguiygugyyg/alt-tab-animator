// ==WindhawkMod==
// @id              alt-tab-animation
// @name            Alt+Tab Smooth Animation
// @description     Smooth customizable entrance animations (Slide, Zoom, Bounce, Fade) for the Windows 11 Alt+Tab switcher.
// @version         1.0.0
// @author          Community
// @github          https://github.com/ramensoftware/windhawk-mods
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -ldwmapi -luser32
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Alt+Tab Smooth Animation for Windows 11

Brings fluid, modern transition animations to the Alt+Tab switcher in Windows 11.

---

### Features & 14 Animation Styles:
Choose from 14 distinct animation styles in the mod settings:

- **Fade + Motion Styles (Smooth & Modern):**
  - **Fade + Slide Up (Default):** Smoothly glides up into place with cubic deceleration while fading in.
  - **Fade + Slide Down:** Gently drops down from above while fading in.
  - **Fade + Slide from Right:** Smooth horizontal entrance sliding from the right.
  - **Fade + Slide from Left:** Smooth horizontal entrance sliding from the left.
  - **Fade + Zoom In (Scale Up):** Scales up into view from the screen center.
  - **Fade + Zoom Out (Drop In):** Drops in from a slightly enlarged scale (108% down to 100%).
  - **Pop & Overshoot (Bouncy Zoom):** Modern iOS/macOS-style pop with an elastic spring bounce.
  - **Bounce & Overshoot (Bouncy Slide Up):** Slides up with an energetic spring effect.
  - **Fade Only:** Pure clean opacity fade without movement.

- **Solid Motion Styles (No Fade / Sharp & Crisp):**
  - **Solid Slide Up:** Crisp mechanical slide up at 100% opacity.
  - **Solid Slide Down:** Crisp mechanical slide down at 100% opacity.
  - **Solid Slide from Right:** Crisp physical slide from the right.
  - **Solid Slide from Left:** Crisp physical slide from the left.
  - **Solid Zoom In:** Crisp physical zoom without opacity fade.

---

### Key Highlights:
- **Zero Input Lag:** The mod intercepts appearance and does not delay window focus or intercept keyboard shortcuts when switching.
- **Rapid Switching Protection:** Multiple quick presses of Alt+Tab seamlessly cancel ongoing animations without visual stutter.
- **Customizable:** Adjust animation duration, slide distance, and zoom scale directly in the Windhawk settings tab.

> **Note on Exit Animation:** In Windows 11, the shell's internal XAML engine immediately unrenders
> switcher visual elements upon releasing the Alt key. Entrance animations work fully, smoothly, and reliably.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- animationStyle: slideUpFade
  $name: Animation style
  $description: Choose from 14 distinct entrance animation styles.
  $options:
  - slideUpFade: Fade + Slide Up (Recommended)
  - slideDownFade: Fade + Slide Down
  - slideLeftFade: Fade + Slide from Right
  - slideRightFade: Fade + Slide from Left
  - zoomInFade: Fade + Zoom In (Scale Up)
  - zoomOutFade: Fade + Zoom Out (Drop Down)
  - popZoom: Pop & Overshoot (Bouncy Zoom)
  - bounceUp: Bounce & Overshoot (Bouncy Slide Up)
  - fadeOnly: Fade Only (No Movement)
  - slideUpSolid: Solid Slide Up (No Fade)
  - slideDownSolid: Solid Slide Down (No Fade)
  - slideLeftSolid: Solid Slide from Right (No Fade)
  - slideRightSolid: Solid Slide from Left (No Fade)
  - zoomSolid: Solid Zoom In (No Fade)

- duration: 150
  $name: Animation duration (ms)
  $description: Duration of the animation in milliseconds (60-350 ms).

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
    AnimStyle style;
    int duration;
    int slideDistance;
    int zoomScale;
} g_settings;

AnimStyle ParseStyle(PCWSTR str) {
    if (!str) return AnimStyle::SlideUpFade;
    if (wcscmp(str, L"slideUpFade") == 0) return AnimStyle::SlideUpFade;
    if (wcscmp(str, L"slideDownFade") == 0) return AnimStyle::SlideDownFade;
    if (wcscmp(str, L"slideLeftFade") == 0) return AnimStyle::SlideLeftFade;
    if (wcscmp(str, L"slideRightFade") == 0) return AnimStyle::SlideRightFade;
    if (wcscmp(str, L"zoomInFade") == 0) return AnimStyle::ZoomInFade;
    if (wcscmp(str, L"zoomOutFade") == 0) return AnimStyle::ZoomOutFade;
    if (wcscmp(str, L"popZoom") == 0) return AnimStyle::PopZoom;
    if (wcscmp(str, L"bounceUp") == 0) return AnimStyle::BounceUp;
    if (wcscmp(str, L"fadeOnly") == 0) return AnimStyle::FadeOnly;
    if (wcscmp(str, L"slideUpSolid") == 0) return AnimStyle::SlideUpSolid;
    if (wcscmp(str, L"slideDownSolid") == 0) return AnimStyle::SlideDownSolid;
    if (wcscmp(str, L"slideLeftSolid") == 0) return AnimStyle::SlideLeftSolid;
    if (wcscmp(str, L"slideRightSolid") == 0) return AnimStyle::SlideRightSolid;
    if (wcscmp(str, L"zoomSolid") == 0) return AnimStyle::ZoomSolid;
    return AnimStyle::SlideUpFade;
}

void LoadSettings() {
    PCWSTR str = Wh_GetStringSetting(L"animationStyle");
    g_settings.style = ParseStyle(str);
    if (str) Wh_FreeStringSetting(str);

    g_settings.duration = Wh_GetIntSetting(L"duration");
    if (g_settings.duration < 40) g_settings.duration = 40;
    if (g_settings.duration > 500) g_settings.duration = 500;

    g_settings.slideDistance = Wh_GetIntSetting(L"slideDistance");
    if (g_settings.slideDistance < 4) g_settings.slideDistance = 4;
    if (g_settings.slideDistance > 160) g_settings.slideDistance = 160;

    g_settings.zoomScale = Wh_GetIntSetting(L"zoomScale");
    if (g_settings.zoomScale < 70) g_settings.zoomScale = 70;
    if (g_settings.zoomScale > 98) g_settings.zoomScale = 98;
}

static std::atomic<uint64_t> g_animGen(0);

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

void StartEntranceAnimation(HWND hWnd, const Settings& s) {
    uint64_t currentGen = ++g_animGen;

    LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_LAYERED)) {
        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
    }
    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, (exStyle | WS_EX_LAYERED) & ~WS_EX_TRANSPARENT);

    bool useFade = HasFade(s.style);
    bool useOvershoot = HasOvershoot(s.style);

    // Initial opacity
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
    AnimStyle style = s.style;

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

            // 1. Opacity
            if (useFade) {
                float fadeEased = EaseOutCubic(progress);
                int alpha = (int)(255.0f * fadeEased + 0.5f);
                if (alpha > 255) alpha = 255;
                SetLayeredWindowAttributes(hWnd, 0, (BYTE)alpha, LWA_ALPHA);
            }

            // 2. Geometry
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

BOOL WINAPI ShowWindow_Hook(HWND hWnd, int nCmdShow) {
    if (IsAltTabWindow(hWnd)) {
        if (nCmdShow == SW_SHOWNA || nCmdShow == SW_SHOW) {
            StartEntranceAnimation(hWnd, g_settings);
            return TRUE;
        } else if (nCmdShow == SW_HIDE) {
            ++g_animGen;
            return pOriginalShowWindow(hWnd, SW_HIDE);
        }
    }
    return pOriginalShowWindow(hWnd, nCmdShow);
}

BOOL Wh_ModInit() {
    Wh_Log(L"[AltTab Animation v1.0.0] Initializing 14 animation styles...");
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

    Wh_Log(L"[AltTab Animation v1.0.0] 14 animation styles active!");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"[AltTab Animation v1.0.0] Unloading mod.");
    ++g_animGen;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}
