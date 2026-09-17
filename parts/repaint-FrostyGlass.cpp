// Theme-specific: moves where FrostyGlass paints the visible bar.
//
// The generic pinning in dock-adaptation.cpp keeps RootGrid tall, because it is
// an ancestor of the icons and pinning it would clip them again. But FrostyGlass
// paints the bar *on* RootGrid, so the paint has to move down to an element that
// is pinned, or the bar is drawn at the full window height.
//
// Only themes that define $Background, $BorderBrush, $BorderThickness and
// $CornerRadius can use this. LiquidGlass2, for one, uses literal values and no
// constants at all, which is why this lives in a per-theme file.
//
// KNOWN BROKEN: the WindhawkBlur in $Background fails to instantiate here -
// "Failed to create proxy brush: 802B000A" - so the bar comes out transparent.
// The same failure also hits the theme's own untouched rule on
// SystemTrayFrameGrid, so the cause is not this file. Under investigation.

    // --- the visible bar itself ---
    //
    // The theme paints the bar on Grid#RootGrid. RootGrid has to stay tall,
    // because it is an ancestor of the icons and pinning it would clip them
    // again, so the painting moves down to TaskbarBackground#BackgroundControl,
    // which is pinned. RootGrid keeps the room and loses the paint; the
    // background element keeps the paint at the stock height.
    //
    // Editing the theme block itself would work too, but it is kept as a
    // verbatim copy of upstream so that a future version can be diffed against
    // it. Adaptation belongs here instead.

    ThemeTargetStyles{L"Taskbar.TaskbarFrame > Grid#RootGrid", {
        L"Background:=Transparent",
        L"BorderThickness=0",
        L"Margin=0",
        L"Padding=0",
        L"VerticalAlignment=Stretch"}},
    ThemeTargetStyles{L"Taskbar.TaskbarBackground#BackgroundControl", {
        L"Height=48",
        L"VerticalAlignment=Bottom",
        L"Margin=0,0,0,4",
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius",
        L"Background:=$Background",
        L"Padding=2,0,1.5,0"}},
