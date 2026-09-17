// Style rules appended to the theme so magnified icons can leave the taskbar.
//
// The taskbar window is made taller by the GetFrameSize hook in glue.cpp, which
// is what gives the icons room. On its own that is not enough: almost everything
// under the frame is VerticalAlignment=Stretch, so the buttons grow with the
// window and a button magnified to 180% overflows again. Measured with the
// window at 128 DIP, Taskbar.TaskListButton went from 48 to 128.
//
// So the elements that provide the room stay tall - the frame, RootGrid, the
// ItemsRepeater, and the ScrollContentPresenter clip that mirrors them - while
// everything visible is pinned to a 48 DIP strip at the bottom. 48 is the stock
// taskbar height, which is the point: the bar, the tray and the clock keep
// looking exactly as they do without the mod, and only the invisible part of the
// window is taller.
//
// These come after the theme's own rules and win over them.

    // --- the icon buttons, which is what the animation scales ---

    ThemeTargetStyles{L"Taskbar.TaskListButton", {
        L"Height=48",
        L"VerticalAlignment=Bottom"}},
    ThemeTargetStyles{L"Taskbar.TaskListLabeledButtonPanel#IconPanel, Grid#IconPanel", {
        L"Height=48",
        L"VerticalAlignment=Bottom"}},
    ThemeTargetStyles{L"Taskbar.ExperienceToggleButton", {
        L"Height=48",
        L"VerticalAlignment=Bottom"}},
    ThemeTargetStyles{L"Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel", {
        L"Height=48",
        L"VerticalAlignment=Bottom"}},

    // --- the system tray, so the clock and the wifi icon keep their size ---

    ThemeTargetStyles{L"SystemTray.SystemTrayFrame", {
        L"Height=48",
        L"VerticalAlignment=Bottom"}},

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
