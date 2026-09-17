"""Switches the mod to a different theme from the pinned upstream styler.

The mod ships one theme rather than the 48 upstream bundles, because the ones
that are never selected are ~10,000 lines of the file and the part of upstream
that churns most. This pulls a different one in without going through them by
hand.

    python scripts/add-theme.py DockLike
    python scripts/add-theme.py --list

It rewrites the theme name in build-mod.py and in the settings dropdown, rebuilds
the mod, and syntax-checks the result.
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "scripts" / "build-mod.py"
HEADER = ROOT / "parts" / "header.cpp"
ADAPTATION = ROOT / "parts" / "dock-adaptation.cpp"
STYLER = ROOT / "upstream" / "windows-11-taskbar-styler@1.10.wh.cpp"


def available_themes(source):
    return sorted(re.findall(r"^const Theme g_theme(\w+) = \{\{", source, re.M))


def current_theme():
    return re.search(r'^THEME = "(\w+)"', BUILD.read_text(encoding="utf-8"), re.M).group(1)


def constants_of(source, theme):
    """The $Constants a theme defines, so we can warn when switching to one that
    is missing the ones the dock adaptation refers to."""
    block = re.search(
        rf"const Theme g_theme{theme} = \{{\{{.*?\}}, \{{(.*?)\}}\}};",
        source,
        re.S,
    )
    if not block:
        return set()
    return set(re.findall(r'L"(\w+)=', block.group(1)))


def main(argv):
    source = STYLER.read_text(encoding="utf-8")
    themes = available_themes(source)

    if len(argv) != 1 or argv[0] in ("-h", "--help"):
        print(__doc__)
        return 1

    if argv[0] == "--list":
        print(f"current: {current_theme()}\n")
        for name in themes:
            print(f"  {name}")
        return 0

    theme = argv[0]
    if theme not in themes:
        print(f"no theme named {theme!r} in {STYLER.name}", file=sys.stderr)
        print("run with --list to see the available ones", file=sys.stderr)
        return 1

    # The dock adaptation repaints the taskbar background onto a pinned element,
    # and refers to the theme's constants to do it. A theme that does not define
    # them would build but come out unstyled in that spot.
    referenced = set(re.findall(r"\$(\w+)", ADAPTATION.read_text(encoding="utf-8")))
    missing = referenced - constants_of(source, theme)
    if missing:
        print(f"warning: {theme} does not define: {', '.join(sorted(missing))}")
        print("         parts/dock-adaptation.cpp refers to them when it repaints")
        print("         the bar background; edit it to match this theme.\n")

    build = BUILD.read_text(encoding="utf-8")
    BUILD.write_text(
        re.sub(r'^THEME = "\w+"', f'THEME = "{theme}"', build, count=1, flags=re.M),
        encoding="utf-8",
    )

    header = HEADER.read_text(encoding="utf-8")
    header = re.sub(r"^- theme: \w+$", f"- theme: {theme}", header, count=1, flags=re.M)
    header = re.sub(
        r'^  - \w+: \w+$', f"  - {theme}: {theme}", header, count=1, flags=re.M
    )
    HEADER.write_text(header, encoding="utf-8")

    print(f"theme set to {theme}, rebuilding...")
    return subprocess.call([sys.executable, str(BUILD)])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
