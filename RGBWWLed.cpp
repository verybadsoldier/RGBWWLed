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
    _animChannelsHsv[CtrlChannel::Hue]->getValue(c.hue);
    _animChannelsHsv[CtrlChannel::Sat]->getValue(c.sat);
    _animChannelsHsv[CtrlChannel::Val]->getValue(c.val);
    _animChannelsHsv[CtrlChannel::ColorTemp]->getValue(c.ct);
}

void RGBWWLed::getAnimChannelRawOutput(ChannelOutput& o) {
    _animChannelsRaw[CtrlChannel::Red]->getValue(o.r);
    _animChannelsRaw[CtrlChannel::Green]->getValue(o.g);
    _animChannelsRaw[CtrlChannel::Blue]->getValue(o.b);
    _animChannelsRaw[CtrlChannel::WarmWhite]->getValue(o.cw);
    _animChannelsRaw[CtrlChannel::ColdWhite]->getValue(o.ww);
}

/**************************************************************
 *                     OUTPUT
 **************************************************************/
bool RGBWWLed::show() {
    bool animFinished = false;
    _queuesFinished.clear();
    switch(_mode) {
    case ColorMode::Hsv:
    {
        for(int i=0; i < _animChannelsHsv.count(); ++i) {
            CtrlChannel ch = _animChannelsHsv.keyAt(i);
            RGBWWAnimatedChannel* pCh = _animChannelsHsv.valueAt(i);
            animFinished |= pCh->process();
            if (pCh->getQueueFinishedNow()) {
                _queuesFinished[ch] = pCh->getValue();
            }
        }

        // now build the output value and set
        HSVCT c;
        getAnimChannelHsvColor(c);

        debugRGBW("NEW: h:%d, s:%d, v:%d, ct: %d\n", c.h, c.s, c.v, c.ct);

        if (getCurrentColor() != c)
            this->setOutput(c);

        break;
    }
    case ColorMode::Raw:
    {
        for(int i=0; i < _animChannelsRaw.count(); ++i) {
            CtrlChannel ch = _animChannelsRaw.keyAt(i);
            RGBWWAnimatedChannel* pCh = _animChannelsRaw.valueAt(i);
            animFinished |= pCh->process();
            if (pCh->getQueueFinishedNow()) {
                _queuesFinished[ch] = pCh->getValue();
            }
        }

        // now build the output value and set
        ChannelOutput o;
        getAnimChannelRawOutput(o);

        debugRGBW("NEWRAW: r:%d, g:%d, b:%d, cw: %d, ww: %d\n", o.r, o.g, o.b, o.cw, o.ww);

        if (getCurrentOutput() != o)
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
    if(_pwm_output != NULL) {
        colorutils.correctBrightness(output);
        _current_output = output;
        debugRGBW("R:%i | G:%i | B:%i | WW:%i | CW:%i", output.r, output.g, output.b, output.ww, output.cw);
        _pwm_output->setOutput(RGBWW_dim_curve[output.r],
                RGBWW_dim_curve[output.g],
                RGBWW_dim_curve[output.b],
                RGBWW_dim_curve[output.ww],
                RGBWW_dim_curve[output.cw]);
    }
};

void RGBWWLed::setOutputRaw(int& red, int& green, int& blue, int& wwhite, int& cwhite) {
    if(_pwm_output != NULL) {
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
    }
    else {
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

//// setHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

bool RGBWWLed::setHSV(const RequestHSVCT& color, QueuePolicy queuePolicy, bool requeue, const String& name) {
    return setHSV(color, 0, queuePolicy, requeue, name);
}

bool RGBWWLed::setHSV(const RequestHSVCT& color, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Hsv;

    bool result = true;
    result &= pushAnimSetAndStay(color.h, time, queuePolicy, CtrlChannel::Hue, requeue, name);
    result &= pushAnimSetAndStay(color.s, time, queuePolicy, CtrlChannel::Sat, requeue, name);
    result &= pushAnimSetAndStay(color.v, time, queuePolicy, CtrlChannel::Val, requeue, name);
    result &= pushAnimSetAndStay(color.ct, time, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
    return result;
}

//// fadeHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

bool RGBWWLed::fadeHSV(const RequestHSVCT& color, int ramp, int direction, bool requeue, const String& name) {
    return fadeHSV( color, ramp, direction, QueuePolicy::Single, requeue, name);
}

bool RGBWWLed::fadeHSV(const RequestHSVCT& color, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
    return fadeHSV( color, ramp, 1, queuePolicy, requeue, name);
}

bool RGBWWLed::fadeHSV(const RequestHSVCT& color, int ramp, int direction, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Hsv;

    bool result = true;
    if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
        result &= pushAnimSetAndStay(color.h, 0, queuePolicy, CtrlChannel::Hue, requeue, name);
        result &= pushAnimSetAndStay(color.s, 0, queuePolicy, CtrlChannel::Sat, requeue, name);
        result &= pushAnimSetAndStay(color.v, 0, queuePolicy, CtrlChannel::Val, requeue, name);
        result &= pushAnimSetAndStay(color.ct, 0, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
    }
    else {
        result &= pushAnimTransitionCircularHue(color.h, ramp, direction, queuePolicy, CtrlChannel::Hue, requeue, name);
        result &= pushAnimTransition(color.s, ramp, queuePolicy, CtrlChannel::Sat, requeue, name);
        result &= pushAnimTransition(color.v, ramp, queuePolicy, CtrlChannel::Val, requeue, name);
        result &= pushAnimTransition(color.ct, ramp, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
    }
}

bool RGBWWLed::fadeHSV(const RequestHSVCT& colorFrom, const RequestHSVCT& color, int ramp, int direction, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Hsv;

    bool result = true;
    if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
        result &= pushAnimSetAndStay(color.h, 0, queuePolicy, CtrlChannel::Hue, requeue, name);
        result &= pushAnimSetAndStay(color.s, 0, queuePolicy, CtrlChannel::Sat, requeue, name);
        result &= pushAnimSetAndStay(color.v, 0, queuePolicy, CtrlChannel::Val, requeue, name);
        result &= pushAnimSetAndStay(color.ct, 0, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
    }
    else {
        result &= pushAnimTransitionCircularHue(colorFrom.h, color.h, ramp, direction, queuePolicy, CtrlChannel::Hue, requeue, name);
        result &= pushAnimTransition(colorFrom.s, color.s, ramp, queuePolicy, CtrlChannel::Sat, requeue, name);
        result &= pushAnimTransition(colorFrom.v, color.v, ramp, queuePolicy, CtrlChannel::Val, requeue, name);
        result &= pushAnimTransition(colorFrom.ct, color.ct, ramp, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
    }
    return result;
}

//// setRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

bool RGBWWLed::setRAW(const RequestChannelOutput& output, QueuePolicy queuePolicy, bool requeue, const String& name) {
    return setRAW(output, 0, queuePolicy, requeue, name);
}

bool RGBWWLed::setRAW(const RequestChannelOutput& output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Raw;

    bool result = true;
    result &= pushAnimSetAndStay(output.r, time, queuePolicy, CtrlChannel::Red, requeue, name);
    result &= pushAnimSetAndStay(output.g, time, queuePolicy, CtrlChannel::Green, requeue, name);
    result &= pushAnimSetAndStay(output.b, time, queuePolicy, CtrlChannel::Blue, requeue, name);
    result &= pushAnimSetAndStay(output.cw, time, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
    result &= pushAnimSetAndStay(output.ww, time, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
}

//// fadeRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

bool RGBWWLed::fadeRAW(const RequestChannelOutput& output, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Raw;

    bool result = true;
    if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
        result &= pushAnimSetAndStay(output.r, 0, queuePolicy, CtrlChannel::Red, requeue, name);
        result &= pushAnimSetAndStay(output.g, 0, queuePolicy, CtrlChannel::Green, requeue, name);
        result &= pushAnimSetAndStay(output.b, 0, queuePolicy, CtrlChannel::Blue, requeue, name);
        result &= pushAnimSetAndStay(output.ww, 0, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
        result &= pushAnimSetAndStay(output.cw, 0, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
    }
    else {
        result &= pushAnimTransition(output.r, ramp, queuePolicy, CtrlChannel::Red, requeue, name);
        result &= pushAnimTransition(output.g, ramp, queuePolicy, CtrlChannel::Green, requeue, name);
        result &= pushAnimTransition(output.b, ramp, queuePolicy, CtrlChannel::Blue, requeue, name);
        result &= pushAnimTransition(output.ww, ramp, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
        result &= pushAnimTransition(output.cw, ramp, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
    }
}

bool RGBWWLed::fadeRAW(const RequestChannelOutput& output_from, const RequestChannelOutput& output, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
    _mode = ColorMode::Raw;

    bool result = true;
    if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
        result &= pushAnimSetAndStay(output.r, 0, queuePolicy, CtrlChannel::Red, requeue, name);
        result &= pushAnimSetAndStay(output.g, 0, queuePolicy, CtrlChannel::Green, requeue, name);
        result &= pushAnimSetAndStay(output.b, 0, queuePolicy, CtrlChannel::Blue, requeue, name);
        result &= pushAnimSetAndStay(output.ww, 0, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
        result &= pushAnimSetAndStay(output.cw, 0, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
    }
    else {
        result &= pushAnimTransition(output_from.r, output.r, ramp, queuePolicy, CtrlChannel::Red, requeue, name);
        result &= pushAnimTransition(output_from.g, output.g, ramp, queuePolicy, CtrlChannel::Green, requeue, name);
        result &= pushAnimTransition(output_from.b, output.b, ramp, queuePolicy, CtrlChannel::Blue, requeue, name);
        result &= pushAnimTransition(output_from.ww, output.ww, ramp, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
        result &= pushAnimTransition(output_from.cw, output.cw, ramp, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
    }
}

bool RGBWWLed::pushAnimSetAndStay(const Optional<AbsOrRelValue>& val, int time, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue())
        return true;
    RGBWWLedAnimation* pAnim = new AnimSetAndStay(val, time, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::pushAnimTransition(const Optional<AbsOrRelValue>& val, int ramp, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue())
        return true;
    RGBWWLedAnimation* pAnim = new AnimTransition(val, ramp, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::pushAnimTransition(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, int ramp, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue())
        return true;
    RGBWWLedAnimation* pAnim = new AnimTransition(from, val, ramp, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::pushAnimTransitionCircularHue(const Optional<AbsOrRelValue>& val, int ramp, int direction, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue()) {
        return true;
    }
    RGBWWLedAnimation* pAnim = new AnimTransitionCircularHue(val, ramp, direction, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::pushAnimTransitionCircularHue(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, int ramp, int direction, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name) {
    if (!val.hasValue())
        return true;
    RGBWWLedAnimation* pAnim = new AnimTransitionCircularHue(from, val, ramp, direction, this, ch, requeue, name);
    return dispatchAnimation(pAnim, ch, queuePolicy);
}

bool RGBWWLed::dispatchAnimation(RGBWWLedAnimation* pAnim, CtrlChannel ch, QueuePolicy queuePolicy, const ChannelList& channels) {
    switch(ch) {
    case CtrlChannel::Hue:
    case CtrlChannel::Sat:
    case CtrlChannel::Val:
    case CtrlChannel::ColorTemp:
        return _animChannelsHsv[ch]->pushAnimation(pAnim, queuePolicy);
        break;
    case CtrlChannel::Red:
    case CtrlChannel::Green:
    case CtrlChannel::Blue:
    case CtrlChannel::WarmWhite:
    case CtrlChannel::ColdWhite:
        return _animChannelsRaw[ch]->pushAnimation(pAnim, queuePolicy);
        break;
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

    for(int i=0; i < group.count(); ++i) {
        if (!all && !channels.contains(group.keyAt(i)))
            continue;
        (group.valueAt(i)->*fnc)();
    }
}

void RGBWWLed::onAnimationFinished(RGBWWLedAnimation* anim) {
}
