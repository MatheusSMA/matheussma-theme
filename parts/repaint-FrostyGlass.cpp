// Theme-specific: moves where FrostyGlass paints the visible bar.
//
// The generic pinning keeps RootGrid tall, because it is an ancestor of the
// icons and pinning it would clip them again. But FrostyGlass paints the bar
// *on* RootGrid, so the paint has to move down onto something pinned - otherwise
// the bar is drawn at the full window height.
//
// It moves onto the Grid inside TaskbarBackground, carrying the theme's own
// property set across unchanged. Grid to Grid matters: the same $Background
// assigned to a Rectangle's Fill instead comes out a different colour than the
// tray, which is a Grid and is painted by the theme directly. Assigning it to
// TaskbarBackground's own Background does not work at all - a WindhawkBlur there
// fails to instantiate, "Failed to create proxy brush: 802B000A".
//
// Only themes defining $Background, $BorderBrush, $BorderThickness and
// $CornerRadius can use this. LiquidGlass2 uses literal values and no constants,
// which is why this is a per-theme file.

    // Width and centring belong here, not on the background element: that
    // control holds only Rectangles, which have no intrinsic width, so sizing it
    // to its content collapses it to zero and nothing is painted at all.
    // The horizontal padding is room, not spacing. Width=Auto makes the frame hug
    // the icons, which also narrows the ScrollContentPresenter's clip onto them -
    // so when the animation shifts neighbours sideways to make way for a magnified
    // icon, they run past the edge and get cut. The padding widens the clip; the
    // painted background below takes a matching margin so it does not cover the
    // extra space. Same split as the vertical fix: what gives room is not what is
    // painted.
    ThemeTargetStyles{L"Taskbar.TaskbarFrame > Grid#RootGrid", {
        L"Background:=Transparent",
        L"BorderThickness=0",
        L"Margin=0",
        L"Padding=48,0,48,0",
        L"Width=Auto",
        L"HorizontalAlignment=Center",
        L"VerticalAlignment=Stretch"}},

    // The theme's RootGrid rule, verbatim, one level down.
    ThemeTargetStyles{L"Taskbar.TaskbarBackground#BackgroundControl > Windows.UI.Xaml.Controls.Grid", {
        L"Margin=48,0,48,4",
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius",
        L"Background:=$Background",
        L"Padding=2,0,1.5,0"}},
