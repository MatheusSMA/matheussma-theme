# MatheusSMA theme

A Windhawk mod for Windows 11 that combines taskbar styling with a macOS-style
dock hover animation in a single mod.

It merges two existing mods:

- **Windows 11 Taskbar Styler** by [m417z](https://github.com/m417z) — the XAML
  Diagnostics styling engine
- **Taskbar Dock Animation** by
  [Ph0en1x-dev](https://github.com/Ph0en1x-dev) — the hover animation

See [`NOTICE.md`](NOTICE.md) for attribution and licensing.

## Why merge them

Run side by side, the two mods have no way to coordinate. Both write to the same
XAML elements: the styler sets geometry (`Width`, `Height`, `Margin`, `Padding`)
and may set `RenderTransform`, while the animation owns `RenderTransform` on the
icons it scales and reads their geometry every frame.

The reason the merge exists is the clipping fix below, which needs a symbol hook
and style rules acting together — neither upstream mod can do both. Two further
problems come from the same lack of coordination and are **not fixed yet**:

**Silent animation loss.** The animation expects a `TransformGroup` with exactly
four children on every element it animates, and bails out quietly when it finds
anything else. A style rule that sets `RenderTransform` on a taskbar button
replaces that group, and the icon stops animating with no error anywhere.

**Per-frame layout polling.** Separately, the animation has no way to learn that
the styler changed the layout, so it hashes the icon host's children on every
rendered frame to detect it. In a single mod the styling engine already knows
when it mutated the visual tree and could say so directly.

## Differences from upstream

- **Two themes.** `FrostyGlass` and `LiquidGlass2`. The other 47 themes bundled
  with the upstream styler are omitted — around 10,000 lines that are never loaded.
  Pristine upstream sources are kept in `upstream/` if you want to port another
  theme in.
- **Defaults changed.** `theme` defaults to `FrostyGlass` and `MaxScale` defaults
  to `180`.
- **Settings keys unchanged.** Both mods' setting names are kept exactly as they
  are upstream, so fixes can be ported without renaming friction. This is why the
  mod mixes camelCase and PascalCase keys.

## Installing

This mod replaces both originals. Install it and then **uninstall
`windows-11-taskbar-styler` and `taskbar-dock-animation`** — running all three at
once duplicates the symbol hooks, and both styling engines compete for the single
XAML diagnostics consumer slot.

1. In Windhawk, choose *Create new mod*.
2. Paste the contents of [`mods/matheussma-theme.wh.cpp`](mods/matheussma-theme.wh.cpp).
3. Compile and install.
4. Uninstall the two upstream mods.

Windows 11 only, x86-64. The animation does not support StartAllBack.

## Building

`mods/matheussma-theme.wh.cpp` is generated. Edit `parts/` and run:

```
python scripts/build-mod.py
```

It assembles the mod from the pinned sources in `upstream/` plus the hand-written
pieces in `parts/`. Every extraction is anchored to an exact line of upstream
text, so an upstream change that moves something fails the build instead of
silently producing a mis-spliced file.

To check it without going through Windhawk's UI, use Windhawk's own compiler:

```
clang++ @"C:\Program Files\Windhawk\Compiler\compile_flags.txt" -I"C:\Program Files\Windhawk\Compiler\include" -fsyntax-only mods/matheussma-theme.wh.cpp
```

To use a different theme from the upstream styler:

```
python scripts/add-theme.py --list
python scripts/add-theme.py DockLike
```

It warns when the new theme does not define the style constants that
`parts/dock-adaptation.cpp` uses to repaint the taskbar background.

## Keeping up with upstream

`upstream/` holds untouched copies of the exact versions this mod was built from.
Diffing a new upstream release against the pinned copy shows what changed in the
engine without the noise of theme changes that do not apply here.

## Known issues

The taskbar window is taller than the visible bar, so it reserves that screen
space: maximized windows stop below the top of the window, not the top of the
bar. `clickThroughTaskbar` is on by default so the invisible strip does not
swallow clicks and hover.

## License

GPLv3. See [`LICENSE`](LICENSE) and [`NOTICE.md`](NOTICE.md).
