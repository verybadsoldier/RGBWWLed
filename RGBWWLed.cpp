/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#include "RGBWWLed.h"
#include "RGBWWAnimatedChannel.h"
#include "RGBWWLedColor.h"
#include "RGBWWLedAnimation.h"
#include "RGBWWLedAnimationQ.h"
#include "RGBWWLedOutput.h"

/**************************************************************
 *                setup, init and settings
 **************************************************************/

RGBWWLed::RGBWWLed() {
    _current_color = HSVCT(0, 0, 0);
    _current_output = ChannelOutput(0, 0, 0, 0, 0);

    _pwm_output = nullptr;

    _animChannelsHsv[CtrlChannel::Hue] = new RGBWWAnimatedChannel(this);
    _animChannelsHsv[CtrlChannel::Sat] = new RGBWWAnimatedChannel(this);
    _animChannelsHsv[CtrlChannel::Val] = new RGBWWAnimatedChannel(this);
    _animChannelsHsv[CtrlChannel::ColorTemp] = new RGBWWAnimatedChannel(this);

    _animChannelsRaw[CtrlChannel::Red] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::Green] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::Blue] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::WarmWhite] = new RGBWWAnimatedChannel(this);
    _animChannelsRaw[CtrlChannel::ColdWhite] = new RGBWWAnimatedChannel(this);
}

RGBWWLed::~RGBWWLed() {
    delete _pwm_output;
    _pwm_output = nullptr;
}

void RGBWWLed::init(int redPIN, int greenPIN, int bluePIN, int wwPIN, int cwPIN, int pwmFrequency /* =200 */) {
    _pwm_output = new PWMOutput(redPIN, greenPIN, bluePIN, wwPIN, cwPIN, pwmFrequency);
}

void RGBWWLed::getAnimChannelHsvColor(HSVCT& c) {
    c.hue = _animChannelsHsv[CtrlChannel::Hue]->getValue();
    c.sat = _animChannelsHsv[CtrlChannel::Sat]->getValue();
    c.val = _animChannelsHsv[CtrlChannel::Val]->getValue();
    c.ct = _animChannelsHsv[CtrlChannel::ColorTemp]->getValue();
}

void RGBWWLed::getAnimChannelRawOutput(ChannelOutput& o) {
    o.r = _animChannelsRaw[CtrlChannel::Red]->getValue();
    o.g = _animChannelsRaw[CtrlChannel::Green]->getValue();
    o.b = _animChannelsRaw[CtrlChannel::Blue]->getValue();
    o.ww = _animChannelsRaw[CtrlChannel::WarmWhite]->getValue();
    o.cw = _animChannelsRaw[CtrlChannel::ColdWhite]->getValue();
}

/**************************************************************
 *                     OUTPUT
 **************************************************************/

bool RGBWWLed::processChannelGroup(const ChannelGroup& cg) {
    bool animFinished = false;
    for (unsigned i = 0; i < cg.count(); ++i) {
        CtrlChannel ch = cg.keyAt(i);
        RGBWWAnimatedChannel* pCh = cg.valueAt(i);
        animFinished |= pCh->process();
    }
    return animFinished;
}

bool RGBWWLed::show() {
    bool animFinished = false;
    switch (_mode) {
    case ColorMode::Hsv: {
        animFinished |= processChannelGroup(_animChannelsHsv);

        HSVCT c;
        getAnimChannelHsvColor(c);

        debug_d("NEW: h:%d, s:%d, v:%d, ct: %d", c.h, c.s, c.v, c.ct);

        this->setOutput(c);

        break;
    }
    case ColorMode::Raw: {
        animFinished |= processChannelGroup(_animChannelsRaw);

        ChannelOutput o;
        getAnimChannelRawOutput(o);

        debug_d("NEWRAW: r:%d, g:%d, b:%d, cw: %d, ww: %d", o.r, o.g, o.b, o.cw, o.ww);

        this->setOutput(o);
        break;
    }
    }

    return animFinished;
}

void RGBWWLed::refresh() {
    setOutput(_current_color);
}

const ChannelOutput& RGBWWLed::getCurrentOutput() const {
    return _current_output;
}

const HSVCT& RGBWWLed::getCurrentColor() const {
    return _current_color;
}

void RGBWWLed::setOutput(HSVCT& outputcolor) {
    RGBWCT rgbwk;
    _current_color = outputcolor;
    colorutils.HSVtoRGB(outputcolor, rgbwk);
    setOutput(rgbwk);
}

void RGBWWLed::setOutput(RGBWCT& outputcolor) {
    ChannelOutput output;
    colorutils.whiteBalance(outputcolor, output);
    setOutput(output);
}

void RGBWWLed::setOutput(ChannelOutput& output) {
    if (_pwm_output != NULL) {
        colorutils.correctBrightness(output);
        _current_output = output;
        debug_d("R:%i | G:%i | B:%i | WW:%i | CW:%i", output.r, output.g, output.b, output.ww, output.cw);
        _pwm_output->setOutput(RGBWW_dim_curve[output.r], RGBWW_dim_curve[output.g], RGBWW_dim_curve[output.b], RGBWW_dim_curve[output.ww],
                RGBWW_dim_curve[output.cw]);
    }
}
;

void RGBWWLed::setOutputRaw(int& red, int& green, int& blue, int& wwhite, int& cwhite) {
    if (_pwm_output != NULL) {
        _current_output = ChannelOutput(red, green, blue, wwhite, cwhite);
        _pwm_output->setOutput(red, green, blue, wwhite, cwhite);
    }
}

/**************************************************************
 *                 ANIMATION/TRANSITION
 **************************************************************/

void RGBWWLed::blink(const ChannelList& channels, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
    if (_mode == ColorMode::Hsv) {
        if (channels.size() == 0 || channels.contains(CtrlChannel::Val))
            _animChannelsHsv[CtrlChannel::Val]->pushAnimation(new AnimBlink(time, this, CtrlChannel::Val, requeue, name), queuePolicy);
        if (channels.contains(CtrlChannel::Sat))
            _animChannelsHsv[CtrlChannel::Sat]->pushAnimation(new AnimBlink(time, this, CtrlChannel::Sat, requeue, name), queuePolicy);
        if (channels.contains(CtrlChannel::Hue))
            _animChannelsHsv[CtrlChannel::Hue]->pushAnimation(new AnimBlink(time, this, CtrlChannel::Hue, requeue, name), queuePolicy);
    } else {
        if (channels.size() == 0 || channels.contains(CtrlChannel::WarmWhite))
            _animChannelsHsv[CtrlChannel::WarmWhite]->pushAnimation(new AnimBlink(time, this, CtrlChannel::WarmWhite, requeue, name), queuePolicy);
        if (channels.size() == 0 || channels.contains(CtrlChannel::ColdWhite))
            _animChannelsHsv[CtrlChannel::ColdWhite]->pushAnimation(new AnimBlink(time, this, CtrlChannel::ColdWhite, requeue, name), queuePolicy);
        if (channels.contains(CtrlChannel::Red))
            _animChannelsHsv[CtrlChannel::Red]->pushAnimation(new AnimBlink(time, this, CtrlChannel::Red, requeue, name), queuePolicy);
        if (channels.contains(CtrlChannel::Green))
            _animChannelsHsv[CtrlChannel::Green]->pushAnimation(new AnimBlink(time, this, CtrlChannel::Green, requeue, name), queuePolicy);
        if (channels.contains(CtrlChannel::Blue))
            _animChannelsHsv[CtrlChannel::Blue]->pushAnimation(new AnimBlink(time, this, CtrlChannel::Blue, requeue, name), queuePolicy);
    }
}

//// fadeHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

bool RGBWWLed::fadeHSV(const RequestHSVCT& color, const RampTimeOrSpeed& ramp, int stay, int direction, bool requeue, const String& name) {
    return fadeHSV(color, ramp, stay, direction, QueuePolicy::Single, requeue, name);
}

bool RGBWWLed::fadeHSV(const RequestHSVCT& color, const RampTimeOrSpeed& ramp, int stay, QueuePolicy queuePolicy, bool requeue, const String& name) {
    return fadeHSV(color, ramp, stay, queuePolicy, requeue, name);
}

bool RGBWWLed::fadeHSV(const RequestHSVCT& color, const RampTimeOrSpeed& ramp, int stay, int direction, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Hsv;

    bool result = true;
    result &= pushAnimTransition(color.h, ramp, stay, direction, queuePolicy, CtrlChannel::Hue, requeue, name);
    result &= pushAnimTransition(color.s, ramp, stay, direction, queuePolicy, CtrlChannel::Sat, requeue, name);
    result &= pushAnimTransition(color.v, ramp, stay, direction, queuePolicy, CtrlChannel::Val, requeue, name);
    result &= pushAnimTransition(color.ct, ramp, stay, direction, queuePolicy, CtrlChannel::ColorTemp, requeue, name);

    return result;
}

bool RGBWWLed::fadeHSV(const RequestHSVCT& colorFrom, const RequestHSVCT& color, const RampTimeOrSpeed& ramp, int stay, int direction, QueuePolicy queuePolicy,
        bool requeue, const String& name) {
    _mode = ColorMode::Hsv;

    bool result = true;

    result &= pushAnimTransition(colorFrom.h, color.h, ramp, stay, direction, queuePolicy, CtrlChannel::Hue, requeue, name);
    result &= pushAnimTransition(colorFrom.s, color.s, ramp, stay, direction, queuePolicy, CtrlChannel::Sat, requeue, name);
    result &= pushAnimTransition(colorFrom.v, color.v, ramp, stay, direction, queuePolicy, CtrlChannel::Val, requeue, name);
    result &= pushAnimTransition(colorFrom.ct, color.ct, ramp, stay, direction, queuePolicy, CtrlChannel::ColorTemp, requeue, name);

    return result;
}

//// fadeRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

bool RGBWWLed::fadeRAW(const RequestChannelOutput& output, const RampTimeOrSpeed& ramp, int stay, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Raw;

    bool result = true;
    result &= pushAnimTransition(output.r, ramp, stay, -1, queuePolicy, CtrlChannel::Red, requeue, name);
    result &= pushAnimTransition(output.g, ramp, stay, -1, queuePolicy, CtrlChannel::Green, requeue, name);
    result &= pushAnimTransition(output.b, ramp, stay, -1, queuePolicy, CtrlChannel::Blue, requeue, name);
    result &= pushAnimTransition(output.ww, ramp, stay, -1, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
    result &= pushAnimTransition(output.cw, ramp, stay, -1, queuePolicy, CtrlChannel::ColdWhite, requeue, name);

    return result;
}

bool RGBWWLed::fadeRAW(const RequestChannelOutput& output_from, const RequestChannelOutput& output, const RampTimeOrSpeed& ramp, int stay, QueuePolicy queuePolicy,
        bool requeue, const String& name) {
    _mode = ColorMode::Raw;

    bool result = true;
    result &= pushAnimTransition(output_from.r, output.r, ramp, stay, -1, queuePolicy, CtrlChannel::Red, requeue, name);
    result &= pushAnimTransition(output_from.g, output.g, ramp, stay, -1, queuePolicy, CtrlChannel::Green, requeue, name);
    result &= pushAnimTransition(output_from.b, output.b, ramp, stay, -1, queuePolicy, CtrlChannel::Blue, requeue, name);
    result &= pushAnimTransition(output_from.ww, output.ww, ramp, stay, -1, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
    result &= pushAnimTransition(output_from.cw, output.cw, ramp, stay, -1, queuePolicy, CtrlChannel::ColdWhite, requeue, name);

    return result;
}

void RGBWWLed::colorDirectHSV(const RequestHSVCT& output) {
    if (output.h.hasValue()) {
        _animChannelsHsv[CtrlChannel::Hue]->setValue(output.h.getValue());
    }
    if (output.s.hasValue()) {
        _animChannelsHsv[CtrlChannel::Sat]->setValue(output.s.getValue());
    }
    if (output.v.hasValue()) {
        _animChannelsHsv[CtrlChannel::Val]->setValue(output.v.getValue());
    }
    if (output.ct.hasValue()) {
        _animChannelsHsv[CtrlChannel::ColorTemp]->setValue(output.ct.getValue());
    }
}

void RGBWWLed::colorDirectRAW(const RequestChannelOutput& output) {
    if (output.r.hasValue()) {
        _animChannelsHsv[CtrlChannel::Red]->setValue(output.r.getValue());
    }
    if (output.g.hasValue()) {
        _animChannelsHsv[CtrlChannel::Green]->setValue(output.g.getValue());
    }
    if (output.b.hasValue()) {
        _animChannelsHsv[CtrlChannel::Blue]->setValue(output.b.getValue());
    }
    if (output.ww.hasValue()) {
        _animChannelsHsv[CtrlChannel::WarmWhite]->setValue(output.ww.getValue());
    }
    if (output.cw.hasValue()) {
        _animChannelsHsv[CtrlChannel::ColdWhite]->setValue(output.cw.getValue());
    }
}

bool RGBWWLed::pushAnimTransition(const Optional<AbsOrRelValue>& val, const RampTimeOrSpeed& ramp, int stay, int direction, QueuePolicy queuePolicy,
        CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue()) {
        return true;
    }
    RGBWWLedAnimation* pAnim = new AnimTransition(val, ramp, stay, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::pushAnimTransition(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, const RampTimeOrSpeed& ramp, int stay, int direction,
        QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue())
        return true;
    RGBWWLedAnimation* pAnim = new AnimTransition(from, val, ramp, stay, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::dispatchAnimation(RGBWWLedAnimation* pAnim, CtrlChannel ch, QueuePolicy queuePolicy, const ChannelList& channels) {
    switch (ch) {
    case CtrlChannel::Hue:
    case CtrlChannel::Sat:
    case CtrlChannel::Val:
    case CtrlChannel::ColorTemp:
        return _animChannelsHsv[ch]->pushAnimation(pAnim, queuePolicy);
    case CtrlChannel::Red:
    case CtrlChannel::Green:
    case CtrlChannel::Blue:
    case CtrlChannel::WarmWhite:
    case CtrlChannel::ColdWhite:
        return _animChannelsRaw[ch]->pushAnimation(pAnim, queuePolicy);
    default:
        return false;
    }
}

void RGBWWLed::clearAnimationQueue(const ChannelList& channels) {
    callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::clearAnimationQueue, channels);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::clearAnimationQueue, channels);
}

void RGBWWLed::skipAnimation(const ChannelList& channels) {
    callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::skipAnimation, channels);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::skipAnimation, channels);
}

void RGBWWLed::pauseAnimation(const ChannelList& channels) {
    callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::pauseAnimation, channels);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::pauseAnimation, channels);
}

void RGBWWLed::continueAnimation(const ChannelList& channels) {
    callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::continueAnimation, channels);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::continueAnimation, channels);
}

void RGBWWLed::callForChannels(const ChannelGroup& group, void (RGBWWAnimatedChannel::*fnc)(), const ChannelList& channels) {
    const bool all = (channels.size() == 0);

    for (unsigned i = 0; i < group.count(); ++i) {
        if (!all && !channels.contains(group.keyAt(i)))
            continue;
        (group.valueAt(i)->*fnc)();
    }
}

void RGBWWLed::onAnimationFinished(const String& name, bool requeued) {
}
