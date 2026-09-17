// Hybrid-specific: the seams between the two themes it is made of.
//
// LiquidGlass2 lays the taskbar out in Grid columns and puts the tray in column
// 2. FrostyGlass does not use columns at all - it just aligns the tray right.
// Borrowing FrostyGlass's tray rules therefore took its looks and dropped its
// position: with no Grid.Column the tray falls into column 0 and lands on the far
// left, where its HorizontalAlignment=Right then aligns it right *within the
// wrong column*.
//
// So the column comes back from the base theme, and the alignment stays Right so
// the tray sits at the far edge rather than LiquidGlass2's inset position.

    ThemeTargetStyles{L"SystemTray.SystemTrayFrame", {
        L"Grid.Column=2",
        L"HorizontalAlignment=Right",
        L"Margin=0,0,12,4"}},

// The pill. LiquidGlass2 derives its radius from an expression over TaskHeight,
// which assumes the height it computes for itself; with the bar pinned to 48 that
// no longer lands on a true pill. Half of 48 is 24, and a radius of exactly half
// the height is what makes the ends semicircular.
//
// Set as a plain number rather than an expression so it does not depend on
// TaskHeight resolving - change it here, or override CornerRadius in the mod's
// Style constants setting to taste.

    ThemeTargetStyles{L"Taskbar.TaskbarFrame > Grid#RootGrid > Taskbar.TaskbarBackground > Grid > Rectangle#BackgroundFill", {
        L"RadiusX=24",
        L"RadiusY=24"}},

    ThemeTargetStyles{L"Taskbar.TaskbarFrame > Grid#RootGrid > Taskbar.TaskbarBackground > Grid > Rectangle#BackgroundStroke", {
        L"RadiusX=24",
        L"RadiusY=24"}},

// The tray's own glass, matched to the bar.
//
// FrostyGlass paints the tray with its $Background - blur 20 over a theme colour
// at 0.15 opacity - which is a much milkier frost than the bar, so the two did
// not read as the same material. These are the bar's own blur values from
// LiquidGlass2, and a radius of 24 to match the bar's pill.
//
// Caveat: the bar paints through a Rectangle's Fill and the tray through a Grid's
// Background, and those two composite differently - identical parameters came out
// as visibly different colours earlier in this theme. If it still reads off,
// nudge the tint here rather than assuming the numbers are wrong.

    ThemeTargetStyles{L"StackPanel#SystemTrayFrameGrid, Grid#SystemTrayFrameGrid", {
        L"Background:=<WindhawkBlur BlurAmount=\"3\" TintColor=\"#14090909\" TintSaturation=\"1.2\"/>",
        L"CornerRadius=24"}},

    ThemeTargetStyles{L"SystemTray.SystemTrayFrame", {
        L"CornerRadius=24"}},
