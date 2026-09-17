// ==WindhawkMod==
// @id              taskbar-tree-probe
// @name            Taskbar Headroom (diagnostic)
// @description     Gives the taskbar window vertical headroom and dumps the resulting layout
// @version         0.6
// @author          MatheusSMA
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject -lshcore -lwindowsapp -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Taskbar Headroom

Test build, step 1 of 2.

Hooks `TaskbarConfiguration::GetFrameSize` to make the taskbar window taller, so
magnified icons have somewhere to go. Measured working: the window went from 48
to 128 DIP.

The bar still looks wrong at this stage, and icons still get cut, because the
buttons stretch to fill the taller frame - so a button magnified to 180% is
bigger too and overflows again. Step 2 pins the visible content back to a short
strip at the bottom.

On the first hover this also writes the resulting layout to
`%TEMP%\taskbar-tree-dump.txt`, which is what step 2's rules get written from.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- FrameHeight: 128
  $name: Frame height (DIP)
  $description: >-
    Absolute height for the taskbar window. The stock height is 48.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>
#undef GetCurrentTime
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <atomic>
#include <cstdio>
#include <string>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Media;

std::atomic<bool> g_taskbarViewDllLoaded = false;
std::atomic<bool> g_dumped = false;
std::atomic<double> g_frameHeight = 128.0;

void LoadSettings() {
    double height = (double)Wh_GetIntSetting(L"FrameHeight");
    g_frameHeight = (height > 0.0) ? height : 128.0;
}

// static double __cdecl TaskbarConfiguration::GetFrameSize(winrt::WindowsUdk::UI::Shell::TaskbarSize)
//
// Returns an absolute height rather than adding to the original: this is called
// several times, nested, during a layout pass, and an additive hook compounded
// itself - a 48 DIP bar became 128 instead of the intended 72.
//
// ponytail: same height for every TaskbarSize class, so the small/large taskbar
// modes collapse to one size. Fine while the mod targets one configuration;
// key off the enum if those modes ever need to differ.
using GetFrameSize_t = double(*)(int);
GetFrameSize_t GetFrameSize_Original;

double GetFrameSize_Hook(int taskbarSize) {
    return g_frameHeight.load();
}

// --- layout dump, so step 2's style rules come from measurements ---

FILE* g_out = nullptr;

void Line(PCWSTR fmt, ...) {
    if (!g_out) return;
    va_list args;
    va_start(args, fmt);
    vfwprintf(g_out, fmt, args);
    va_end(args);
    fwprintf(g_out, L"\n");
    fflush(g_out);
}

void DescribeElement(PCWSTR prefix, FrameworkElement const& e) {
    if (!e) {
        Line(L"%s<null>", prefix);
        return;
    }

    std::wstring className{winrt::get_class_name(e)};
    std::wstring name{e.Name()};

    std::wstring clipText = L"Clip=none";
    if (auto clip = e.Clip()) {
        auto r = clip.Rect();
        WCHAR buf[128];
        swprintf_s(buf, L"Clip=(%.1f,%.1f %.1fx%.1f)", r.X, r.Y, r.Width, r.Height);
        clipText = buf;
    }

    Line(L"%s%s  name='%s'  size=%.1fx%.1f  align=%d  %s",
         prefix, className.c_str(), name.c_str(),
         e.ActualWidth(), e.ActualHeight(),
         (int)e.VerticalAlignment(), clipText.c_str());
}

FrameworkElement FindDescendant(FrameworkElement const& root, PCWSTR needle) {
    if (!root) return nullptr;

    int count = VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; i++) {
        auto child = VisualTreeHelper::GetChild(root, i).try_as<FrameworkElement>();
        if (!child) continue;

        std::wstring className{winrt::get_class_name(child)};
        if (className.find(needle) != std::wstring::npos) return child;

        if (auto found = FindDescendant(child, needle)) return found;
    }
    return nullptr;
}

// Everything under the frame, so step 2 can see which elements stretched to fill
// the taller window and which stayed put.
void DumpSubtree(FrameworkElement const& element, int depth) {
    if (!element || depth > 6) return;

    WCHAR prefix[64];
    swprintf_s(prefix, L"%*s", depth * 2, L"");
    std::wstring indent = std::wstring(prefix) + L"- ";
    DescribeElement(indent.c_str(), element);

    int count = VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < count; i++) {
        if (auto child = VisualTreeHelper::GetChild(element, i).try_as<FrameworkElement>()) {
            DumpSubtree(child, depth + 1);
        }
    }
}

void DumpTree(FrameworkElement const& taskbarFrame) {
    WCHAR tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath)) return;
    std::wstring path = std::wstring(tempPath) + L"taskbar-tree-dump.txt";
    g_out = _wfopen(path.c_str(), L"w, ccs=UTF-8");
    if (!g_out) return;

    Line(L"=== taskbar layout at frame height %.0f ===", g_frameHeight.load());
    Line(L"(align: 0=Top 1=Center 2=Bottom 3=Stretch)");
    Line(L"");

    Line(L"--- ancestry above the frame (where the clipping lives) ---");
    DependencyObject current = taskbarFrame;
    for (int depth = 0; current && depth < 20; depth++) {
        if (auto element = current.try_as<FrameworkElement>()) {
            WCHAR prefix[64];
            swprintf_s(prefix, L"[%02d] ", depth);
            DescribeElement(prefix, element);
        }
        current = VisualTreeHelper::GetParent(current);
    }

    Line(L"");
    Line(L"--- everything under the frame (what stretched) ---");
    DumpSubtree(taskbarFrame, 0);

    Line(L"");
    Line(L"=== end ===");

    fclose(g_out);
    g_out = nullptr;
}

using TaskbarFrame_OnPointerMoved_t = int(WINAPI*)(void*, void*);
TaskbarFrame_OnPointerMoved_t TaskbarFrame_OnPointerMoved_Original;

int WINAPI TaskbarFrame_OnPointerMoved_Hook(void* pThis, void* pArgs) {
    auto original = [=]() { return TaskbarFrame_OnPointerMoved_Original(pThis, pArgs); };

    if (g_dumped.load()) return original();

    FrameworkElement element = nullptr;
    ((IUnknown*)pThis)->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                       winrt::put_abi(element));
    if (!element) return original();

    if (winrt::get_class_name(element) != L"Taskbar.TaskbarFrame") return original();

    if (!g_dumped.exchange(true)) {
        try {
            DumpTree(element);
            Wh_Log(L"Headroom: layout dumped");
        } catch (...) {
            Wh_Log(L"Headroom: dump failed");
        }
    }

    return original();
}

HMODULE GetTaskbarViewModuleHandle() {
    HMODULE module = GetModuleHandle(L"Taskbar.View.dll");
    if (!module) module = GetModuleHandle(L"ExplorerExtensions.dll");
    return module;
}

bool HookTaskbarViewDllSymbols(HMODULE module) {
    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {
            {LR"(public: static double __cdecl winrt::Taskbar::implementation::TaskbarConfiguration::GetFrameSize(enum winrt::WindowsUdk::UI::Shell::TaskbarSize))"},
            &GetFrameSize_Original,
            GetFrameSize_Hook,
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::Taskbar::implementation::TaskbarFrame,struct winrt::Windows::UI::Xaml::Controls::IControlOverrides>::OnPointerMoved(void *))"},
            &TaskbarFrame_OnPointerMoved_Original,
            TaskbarFrame_OnPointerMoved_Hook,
        },
    };

    if (!HookSymbols(module, hooks, ARRAYSIZE(hooks))) {
        Wh_Log(L"Headroom: HookSymbols failed");
        return false;
    }

    Wh_Log(L"Headroom: hooked, frame height %.0f", g_frameHeight.load());
    return true;
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original;

HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
    HMODULE module = LoadLibraryExW_Original(lpLibFileName, hFile, dwFlags);
    if (!module) return module;

    if (!g_taskbarViewDllLoaded && GetTaskbarViewModuleHandle() == module) {
        if (!g_taskbarViewDllLoaded.exchange(true)) {
            if (HookTaskbarViewDllSymbols(module)) Wh_ApplyHookOperations();
        }
    }
    return module;
}

BOOL Wh_ModInit() {
    Wh_Log(L"Headroom: Wh_ModInit");
    LoadSettings();

    if (HMODULE taskbarViewModule = GetTaskbarViewModuleHandle()) {
        g_taskbarViewDllLoaded = true;
        if (!HookTaskbarViewDllSymbols(taskbarViewModule)) return FALSE;
    } else {
        HMODULE kernelBaseModule = GetModuleHandle(L"kernelbase.dll");
        auto pLoadLibraryExW =
            (decltype(&LoadLibraryExW))GetProcAddress(kernelBaseModule, "LoadLibraryExW");
        WindhawkUtils::SetFunctionHook(pLoadLibraryExW, LoadLibraryExW_Hook,
                                       &LoadLibraryExW_Original);
    }
    return TRUE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}
