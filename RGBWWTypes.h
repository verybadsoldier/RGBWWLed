#pragma once

enum class ColorMode {
	Hsv,
	Raw,
};

enum class CtrlChannel {
	None,

    Hue,
    Sat,
    Val,
    ColorTemp,

    Red,
    Green,
    Blue,
    ColdWhite,
    WarmWhite,
};

struct BresenhamValues {
    int delta, error, count, step;
};

enum class QueuePolicy {
    FrontReset, // queue to front and let the original anim run from the beginning afterwards
    Front, // queue to front and the current animation will continue where it was interrupted
    Back,
    Single,
};

