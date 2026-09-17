// Theme-specific: moves where FrostyGlass paints the visible bar.
//
// The generic pinning in dock-adaptation.cpp keeps RootGrid tall, because it is
// an ancestor of the icons and pinning it would clip them again. But FrostyGlass
// paints the bar *on* RootGrid, so the paint has to move down onto something
// that is pinned - otherwise the bar is drawn at the full window height.
//
// The paint goes on the Rectangles inside TaskbarBackground, using Fill and
// Stroke, NOT on TaskbarBackground's own Background property. A WindhawkBlur
// assigned to that Control's Background fails to instantiate - "Failed to create
// proxy brush: 802B000A" - and the bar comes out empty. LiquidGlass2 paints the
// same element this way with 28 blurs and renders correctly, which is where this
// shape comes from.
//
// FrostyGlass collapses both Rectangles because it paints on RootGrid instead,
// so they have to be made visible again here. These rules come after the theme's
// own and override them.
//
// Only themes defining $Background, $BorderBrush, $BorderThickness and
// $CornerRadius can use this. LiquidGlass2 uses literal values and no constants
// at all, which is why this is a per-theme file.

    // --- the visible bar itself ---

    // Width=Auto and Center go here, not on the background element below.
    // TaskbarBackground contains only Rectangles, which have no intrinsic width,
    // so asking it to size to its content collapses it to zero and nothing is
    // painted at all. RootGrid hugs the icons and the background stretches inside
    // it - which is how LiquidGlass2 does it, and that theme renders correctly.
    ThemeTargetStyles{L"Taskbar.TaskbarFrame > Grid#RootGrid", {
        L"Background:=Transparent",
        L"BorderThickness=0",
        L"Margin=0",
        L"Padding=0",
        L"Width=Auto",
        L"HorizontalAlignment=Center",
        L"VerticalAlignment=Stretch"}},

    ThemeTargetStyles{L"Taskbar.TaskbarBackground#BackgroundControl", {
        L"Height=48",
        L"VerticalAlignment=Bottom",
        L"Margin=0,0,0,4",
        L"Padding=2,0,1.5,0"}},

    ThemeTargetStyles{L"Taskbar.TaskbarBackground#BackgroundControl > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Shapes.Rectangle#BackgroundFill", {
        L"Visibility=Visible",
        L"Fill:=$Background",
        L"RadiusX=$CornerRadius",
        L"RadiusY=$CornerRadius"}},

    ThemeTargetStyles{L"Taskbar.TaskbarBackground#BackgroundControl > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Shapes.Rectangle#BackgroundStroke", {
        L"Visibility=Visible",
        L"Stroke:=$BorderBrush",
        L"StrokeThickness=$BorderThickness",
        L"RadiusX=$CornerRadius",
        L"RadiusY=$CornerRadius",
        L"VerticalAlignment=Stretch",
        L"HorizontalAlignment=Stretch",
        L"Height=NaN"}},
