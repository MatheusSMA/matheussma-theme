"""Assembles mods/matheussma-theme.wh.cpp out of the pinned upstream sources.

A Windhawk mod has to be a single .cpp file, and this one is ~11,700 lines drawn
from two upstream mods. Copying that by hand is how transcription bugs get in, so
the machine does the copying and the hand-written parts live in parts/.

Every extraction is anchored to an exact line of text rather than a line number.
If an upstream update moves something, the anchor stops matching and this fails
loudly, which is the point - a silent mis-slice would produce a file that looks
fine and behaves wrong.

Run it again after replacing a file in upstream/, or after editing parts/.

    python scripts/build-mod.py
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UPSTREAM = ROOT / "upstream"
PARTS = ROOT / "parts"
OUT = ROOT / "mods" / "matheussma-theme.wh.cpp"

STYLER = UPSTREAM / "windows-11-taskbar-styler@1.10.wh.cpp"
DOCK = UPSTREAM / "taskbar-dock-animation@1.9.2.wh.cpp"

# The themes to include. The rest stay in upstream/ and are pulled in with
# scripts/add-theme.py. Each one carries ~200-400 lines, and the ones nobody
# selects are the bulk of the upstream file.
THEMES = ["FrostyGlass", "LiquidGlass2"]


class BuildError(Exception):
    pass


def read(path):
    if not path.exists():
        raise BuildError(f"missing input: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8").splitlines()


def find_line(lines, needle, *, start=0, what=""):
    """Index of the first line equal to `needle` after `start`.

    Exact match, not substring: anchors have to be unambiguous or they are not
    anchors.
    """
    for i in range(start, len(lines)):
        if lines[i] == needle:
            return i
    raise BuildError(
        f"anchor not found ({what or needle!r}).\n"
        f"  Upstream probably changed. Re-check the line and update this script."
    )


def slice_between(lines, first, last, *, what, inclusive_last=True):
    a = find_line(lines, first, what=f"{what} (start)")
    b = find_line(lines, last, start=a, what=f"{what} (end)")
    return lines[a : b + 1 if inclusive_last else b]


def section(title):
    bar = "=" * 70
    return [
        "",
        f"// {bar}",
        f"// {title}",
        f"// {bar}",
        "",
    ]


# --------------------------------------------------------------------------
# styler
# --------------------------------------------------------------------------


def styler_theme_structs(lines):
    return slice_between(
        lines,
        "struct ThemeTargetStyles {",
        "};",
        what="ThemeTargetStyles struct",
    ) + slice_between(
        lines,
        "struct Theme {",
        "};",
        what="Theme struct",
    )


def theme_adaptation(generic, name):
    """The rules appended to one theme: the generic pinning, plus that theme's
    repaint file if it has one.

    The repaint is per-theme because it refers to the theme's own $constants, and
    not every theme defines them.
    """
    specific = PARTS / f"repaint-{name}.cpp"
    return generic + (read(specific) if specific.exists() else [])


def styler_theme(lines, name, adaptation):
    """One theme definition, with the dock adaptation rules appended to its
    target styles.

    The adaptation goes at the end of that list, so it is applied after the
    theme's own rules and overrides them, while the theme text itself stays a
    verbatim copy of upstream for diffing against future versions.
    """
    start = find_line(lines, f"const Theme g_theme{name} = {{{{", what=f"theme {name}")

    end = None
    for i in range(start, len(lines)):
        if lines[i] == "}};":
            end = i
            break
    if end is None:
        raise BuildError(f"theme {name} is never closed")

    block = lines[start : end + 1]

    # A theme is `{{ <target styles> }, { <constants> }}`, except that the
    # constants section is optional - LiquidGlass2 uses literal values and has
    # none, so its block runs straight from the styles to `}};`. The adaptation
    # goes at the end of the target styles either way.
    split_at = len(block) - 1
    for i, line in enumerate(block):
        if line == "}, {":
            split_at = i

    return block[:split_at] + adaptation + block[split_at:]


def dock_settings(lines):
    """The animation's settings block, copied verbatim rather than retyped."""
    a = find_line(lines, "// ==WindhawkModSettings==", what="dock settings start")
    b = find_line(lines, "// ==/WindhawkModSettings==", start=a,
                  what="dock settings end")
    body = lines[a + 1 : b]
    # Drop the /* */ that wrap it: the merged settings block has its own.
    return [ln for ln in body if ln.strip() not in ("/*", "*/")]


def splice_dock_settings(header, settings):
    marker = "// @@DOCK_SETTINGS@@"
    a = find_line(header, marker, what="dock settings placeholder")
    return header[:a] + settings + header[a + 1 :]


def styler_engine(lines):
    """Everything from the end of the theme blobs to just before the Windhawk
    entry points, which get rewritten.

    Anchored on the first global after the themes rather than on the first
    function: a block of state and string constants sits between them, and
    starting at the function left all of it behind.
    """
    a = find_line(lines, "std::atomic<bool> g_initialized;", what="engine start")
    b = find_line(lines, "BOOL Wh_ModInit() {", start=a, what="styler entry points")
    return lines[a:b]


def styler_entry_points(lines):
    """The styler's four callbacks, renamed so the merged mod can own the real
    ones and call these in a defined order."""
    a = find_line(lines, "BOOL Wh_ModInit() {", what="styler Wh_ModInit")
    body = lines[a:]

    renames = {
        "BOOL Wh_ModInit() {": "BOOL Styler_Init() {",
        "void Wh_ModAfterInit() {": "void Styler_AfterInit() {",
        "void Wh_ModUninit() {": "void Styler_Uninit() {",
        "void Wh_ModSettingsChanged() {": "void Styler_SettingsChanged() {",
    }
    seen = set()
    out = []
    for line in body:
        if line in renames:
            seen.add(line)
            out.append(renames[line])
        else:
            out.append(line)

    missing = set(renames) - seen
    if missing:
        raise BuildError(f"styler entry points not found: {sorted(missing)}")
    return out


def theme_options(themes):
    """The dropdown entries for the settings, generated from the theme list so
    the list and the dropdown cannot drift apart."""
    return [f"  - {name}: {name}" for name in themes]


def splice_theme_options(header, themes):
    marker = "// @@THEME_OPTIONS@@"
    a = find_line(header, marker, what="theme options placeholder")
    return header[:a] + theme_options(themes) + header[a + 1 :]


def patch_theme_lookup(engine, themes):
    """Replace the 49-branch if/else that maps a theme name to its object.

    Only the included themes ship, so the chain is rebuilt for those. Leaving the
    full chain would reference objects that are not in the file.
    """
    first = '    if (wcscmp(themeName, L"TranslucentTaskbar") == 0) {'
    a = find_line(engine, first, what="theme lookup chain")

    end_anchor = "    Wh_FreeStringSetting(themeName);"
    b = find_line(engine, end_anchor, start=a, what="end of theme lookup chain")

    replacement = []
    for i, name in enumerate(themes):
        keyword = "if" if i == 0 else "} else if"
        replacement.append(f'    {keyword} (wcscmp(themeName, L"{name}") == 0) {{')
        replacement.append(f"        theme = &g_theme{name};")
    replacement.append("    }")
    return engine[:a] + replacement + engine[b:]


def patch_shared_library_hook(engine):
    """Both upstream mods hook kernelbase!LoadLibraryExW. One function can only
    carry one hook, so the styler's keeps the hook and tells the animation."""
    anchor = "    return module;"
    start = find_line(
        engine,
        "HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR lpLibFileName,",
        what="styler LoadLibraryExW hook",
    )
    a = find_line(engine, anchor, start=start, what="end of LoadLibraryExW hook")

    injected = [
        "",
        "    // The animation and the clipping fix both wait for Taskbar.View.dll, and",
        "    // a second hook on this function would replace this one rather than",
        "    // chain, so they are told from here instead.",
        "    dock::OnModuleLoaded();",
        "    Clipping_OnModuleLoaded();",
    ]
    return engine[:a] + injected + engine[a:]


# --------------------------------------------------------------------------
# animation
# --------------------------------------------------------------------------


def dock_includes(lines):
    """The animation's #include block, which has to stay at file scope.

    Includes inside `namespace dock { }` would put every declaration in those
    headers into dock::, so they are hoisted out and the namespace starts after
    them.
    """
    a = find_line(lines, "#include <windhawk_utils.h>", what="dock includes")
    b = find_line(lines, "using namespace winrt::Windows::UI::Xaml;", start=a,
                  what="end of dock includes")
    return [ln for ln in lines[a:b] if ln.startswith("#") or not ln.strip()]


def dock_code(lines):
    """Everything from the animation's using-directives onward. Those are
    scoped, so they belong inside the namespace rather than at file scope."""
    a = find_line(lines, "using namespace winrt::Windows::UI::Xaml;",
                  what="dock code start")
    return lines[a:]


def patch_dock(body):
    renames = {
        "BOOL Wh_ModInit() {": "BOOL Init() {",
        "void Wh_ModAfterInit() {": "void AfterInit() {",
        "void Wh_ModBeforeUninit() {": "void BeforeUninit() {",
        "void Wh_ModSettingsChanged() {": "void SettingsChanged() {",
    }
    seen = set()
    out = []
    for line in body:
        if line in renames:
            seen.add(line)
            out.append(renames[line])
        else:
            out.append(line)

    missing = set(renames) - seen
    if missing:
        raise BuildError(f"dock entry points not found: {sorted(missing)}")

    out = rename_run_from_window_thread(out)
    out = drop_dock_library_hook(out)
    return out


def rename_run_from_window_thread(body):
    """Both halves define RunFromWindowThread, and the namespace is not enough.

    Its arguments are types from the global namespace, so argument-dependent
    lookup pulls the styler's overload in as a candidate even from inside
    namespace dock, and the call is ambiguous. A distinct name avoids the whole
    question.
    """
    text = "\n".join(body)
    if "RunFromWindowThread" not in text:
        raise BuildError("expected the animation to define RunFromWindowThread")
    return text.replace("RunFromWindowThread", "RunOnWindowThread").split("\n")


def drop_dock_library_hook(body):
    """Remove the animation's own LoadLibraryExW hook installation.

    The shared hook in the styler calls OnModuleLoaded() instead. The hook
    function itself is kept and renamed, so the detection logic stays in one
    place rather than being duplicated into the styler.
    """
    text = "\n".join(body)

    old_install = re.search(
        r"[ \t]*\} else \{\n"
        r"[ \t]*HMODULE kernelBaseModule = GetModuleHandle\(L\"kernelbase\.dll\"\);\n"
        r"(?:.*\n)*?"
        r"[ \t]*&LoadLibraryExW_Original\);\n"
        r"[ \t]*\}\n",
        text,
    )
    if not old_install:
        raise BuildError(
            "the animation's LoadLibraryExW installation was not found; "
            "check whether upstream restructured Wh_ModInit"
        )

    text = text.replace(
        old_install.group(0),
        "    }\n",
    )

    # Turn the hook into a plain notification the shared hook can call.
    old_hook = re.search(
        r"HMODULE WINAPI LoadLibraryExW_Hook\(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags\) \{\n"
        r"[ \t]*HMODULE module = LoadLibraryExW_Original\(lpLibFileName, hFile, dwFlags\);\n"
        r"[ \t]*if \(!module\) return module;\n",
        text,
    )
    if not old_hook:
        raise BuildError("the animation's LoadLibraryExW_Hook body changed shape")

    text = text.replace(
        old_hook.group(0),
        "// Called by the shared LoadLibraryExW hook in the styler half.\n"
        "void OnModuleLoaded() {\n",
    )

    # It used to receive `module` from the hook's arguments; now it looks it up.
    text = text.replace(
        "    if (!g_taskbarViewDllLoaded && GetTaskbarViewModuleHandle() == module) {",
        "    HMODULE module = GetTaskbarViewModuleHandle();\n"
        "    if (module && !g_taskbarViewDllLoaded) {",
    )

    # And it used to return that module. Scoped to this function's own body:
    # a global substitution would hit whichever `return module;` came first.
    text = strip_return_module(text, "void OnModuleLoaded() {")

    return text.split("\n")


def strip_return_module(text, signature):
    """Remove `return module;` from the body of one function."""
    start = text.index(signature)
    end = text.index("\n}\n", start) + len("\n}\n")

    body = text[start:end]
    stripped = re.sub(r"[ \t]*return module;\n", "", body)
    if stripped == body:
        raise BuildError(f"no `return module;` to remove from {signature!r}")

    return text[:start] + stripped + text[end:]

    # The using-declaration and original pointer belong to the styler's hook now.
    text = text.replace("using LoadLibraryExW_t = decltype(&LoadLibraryExW);\n", "")
    text = text.replace("LoadLibraryExW_t LoadLibraryExW_Original;\n", "")

    return text.split("\n")


def main():
    try:
        styler = read(STYLER)
        dock = read(DOCK)

        header = read(PARTS / "header.cpp")
        glue = read(PARTS / "glue.cpp")
        adaptation = read(PARTS / "dock-adaptation.cpp")

        header = splice_dock_settings(header, dock_settings(dock))
        header = splice_theme_options(header, THEMES)

        engine = styler_engine(styler)
        engine = patch_theme_lookup(engine, THEMES)
        engine = patch_shared_library_hook(engine)

        out = []
        out += header
        out += dock_includes(dock)

        out += section("Theme data, from the Windows 11 Taskbar Styler by m417z")
        out += styler_theme_structs(styler)
        out += ["", "// clang-format off", ""]
        for name in THEMES:
            out += styler_theme(styler, name, theme_adaptation(adaptation, name))
            out += [""]
        out += ["// clang-format on", ""]

        out += section(
            "Styling engine, from the Windows 11 Taskbar Styler by m417z (GPLv3)"
        )
        out += [
            "// The shared LoadLibraryExW hook below lives in this half and calls into",
            "// the animation and the clipping fix, both defined after it.",
            "namespace dock {",
            "void OnModuleLoaded();",
            "}",
            "void Clipping_OnModuleLoaded();",
            "",
        ]
        out += engine
        out += styler_entry_points(styler)

        out += section(
            "Animation, from Taskbar Dock Animation by Ph0en1x-dev (MIT)"
        )
        out += [
            "// Namespaced because it collides with the styler on LoadSettings,",
            "// RunFromWindowThread and LoadLibraryExW_Hook. The styler half cannot be",
            "// namespaced in turn: it reopens namespace winrt and specializes",
            "// winrt::impl::guid_v, neither of which is legal from inside another",
            "// namespace.",
            "namespace dock {",
            "",
        ]
        out += patch_dock(dock_code(dock))
        out += ["", "}  // namespace dock", ""]

        out += section("Merged entry points and the clipping fix")
        out += glue

        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_text("\n".join(out) + "\n", encoding="utf-8")

    except BuildError as e:
        print(f"build failed: {e}", file=sys.stderr)
        return 1

    print(f"wrote {OUT.relative_to(ROOT)} ({len(out)} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
