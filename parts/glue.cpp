// ---------------------------------------------------------------------------
// Clipping fix, part 1: room
// ---------------------------------------------------------------------------
//
// Part 2 is in parts/dock-adaptation.cpp, as style rules appended to the theme.
// Neither half works without the other: this one gives the icons somewhere to
// go, and the rules keep the buttons from growing into it.
//
// XAML is not composited outside its host window, so with the stock 48 DIP
// taskbar an icon magnified to 180% has nowhere to be drawn. Resizing elements
// or opening the ScrollContentPresenter's clip does nothing, because all of it
// happens inside a window that is still 48 DIP - the clip mirrors the window and
// follows it on its own.

std::atomic<double> g_frameHeight = 96.0;
std::atomic<bool> g_frameSizeHooked = false;

void Clipping_LoadSettings() {
    double height = (double)Wh_GetIntSetting(L"frameHeight");
    g_frameHeight = (height > 0.0) ? height : 96.0;
}

// static double __cdecl
//   TaskbarConfiguration::GetFrameSize(winrt::WindowsUdk::UI::Shell::TaskbarSize)
using GetFrameSize_t = double (*)(int);
GetFrameSize_t GetFrameSize_Original;

double GetFrameSize_Hook(int taskbarSize) {
    // Absolute, not added to the original: this runs several times, nested,
    // during a layout pass, and adding to the result compounded - a 48 DIP bar
    // came out at 128 instead of the intended 72.
    //
    // ponytail: the same height for every TaskbarSize, so the small and large
    // taskbar modes collapse into one. Key off the enum if they ever need to
    // differ.
    return g_frameHeight.load();
}

void Clipping_OnModuleLoaded() {
    if (g_frameSizeHooked.load()) {
        return;
    }

    HMODULE module = dock::GetTaskbarViewModuleHandle();
    if (!module) {
        return;
    }

    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {
            {LR"(public: static double __cdecl winrt::Taskbar::implementation::TaskbarConfiguration::GetFrameSize(enum winrt::WindowsUdk::UI::Shell::TaskbarSize))"},
            &GetFrameSize_Original,
            GetFrameSize_Hook,
        },
    };

    if (!HookSymbols(module, hooks, ARRAYSIZE(hooks))) {
        // Not fatal. Everything else still works; the icons just get cut off
        // again, which is the behaviour of the upstream mods.
        Wh_Log(L"GetFrameSize not found - magnified icons will be clipped");
        return;
    }

    g_frameSizeHooked = true;
    Wh_Log(L"Taskbar window height set to %.0f DIP", g_frameHeight.load());
}

// ---------------------------------------------------------------------------
// Windhawk entry points
// ---------------------------------------------------------------------------
//
// One of each, driving both halves in a defined order. The upstream callbacks
// are renamed by scripts/build-mod.py: the styler's to Styler_*, the animation's
// into dock::.

BOOL Wh_ModInit() {
    Wh_Log(L">");

    Clipping_LoadSettings();

    // The styler goes first because it owns the shared kernelbase!LoadLibraryExW
    // hook. Both upstream mods install one, a function can only carry one, and
    // the animation waits on that hook to learn that Taskbar.View.dll arrived.
    if (!Styler_Init()) {
        return FALSE;
    }

    if (!dock::Init()) {
        return FALSE;
    }

    // If Taskbar.View.dll is already loaded, hook it now; otherwise the shared
    // LoadLibraryExW hook calls this again when it shows up.
    Clipping_OnModuleLoaded();

    return TRUE;
}

void Wh_ModAfterInit() {
    Wh_Log(L">");

    Styler_AfterInit();
    dock::AfterInit();
}

void Wh_ModBeforeUninit() {
    Wh_Log(L">");

    // Runs while the hooks are still live, which is what lets the animation put
    // the icons back. Without it they stay frozen mid-scale on the taskbar.
    dock::BeforeUninit();
}

void Wh_ModUninit() {
    Wh_Log(L">");

    Styler_Uninit();
}

void Wh_ModSettingsChanged() {
    Wh_Log(L">");

    Clipping_LoadSettings();

    // The animation first: it reads icon geometry that the styler is about to
    // re-apply styles over.
    dock::SettingsChanged();
    Styler_SettingsChanged();
}
