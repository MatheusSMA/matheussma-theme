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

Two problems follow from that, and both are fixed by having one mod own both
sides:

**Silent animation loss.** The animation expects a `TransformGroup` with exactly
four children on every element it animates, and bails out quietly when it finds
anything else. A style rule that sets `RenderTransform` on a taskbar button
replaces that group, and the icon stops animating with no error anywhere. Here,
the styling engine refuses to write `RenderTransform` to an element the animation
owns, and logs when it skips one.

**Per-frame layout polling.** Separately, the animation has no way to learn that
the styler changed the layout, so it hashes the icon host's children on every
rendered frame to detect it. In a single mod the styling engine already knows
when it mutated the visual tree, so it marks the geometry dirty and the animation
re-measures on the next frame. The per-frame hash is gone.

## Differences from upstream

- **One theme.** Only `FrostyGlass` is included. The other 48 themes bundled with
  the upstream styler are omitted — around 10,000 lines that are never loaded.
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

## Keeping up with upstream

`upstream/` holds untouched copies of the exact versions this mod was built from.
`scripts/check-upstream.ps1` fetches the current upstream source and diffs it
against the pinned copy, so engine fixes can be found and ported without wading
through theme changes that do not apply here.

## Known issues

Inherited from the upstream animation: icons can be clipped by the taskbar at
high scale values. Upstream recommends keeping `MaxScale` at or below 130; this
mod ships 180, so clipping is expected until it is fixed here.

## License

GPLv3. See [`LICENSE`](LICENSE) and [`NOTICE.md`](NOTICE.md).
