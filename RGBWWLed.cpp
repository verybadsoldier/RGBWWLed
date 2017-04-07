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
	switch(_mode) {
	case ColorMode::Hsv:
	{
	    for(int i=0; i < _animChannelsHsv.count(); ++i) {
	    	_animChannelsHsv.valueAt(i)->process();
	    }

	    // now build the output value and set
	    HSVCT c;
	    getAnimChannelHsvColor(c);

	    Serial.printf("NEW: h:%d, s:%d, v:%d, ct: %d\n", c.h, c.s, c.v, c.ct);

	    if (getCurrentColor() != c) {
	    	this->setOutput(c);
	    }
		break;
	}
	case ColorMode::Raw:
	{
	    for(int i=0; i < _animChannelsRaw.count(); ++i) {
	    	_animChannelsRaw.valueAt(i)->process();
	    }

	    // now build the output value and set
	    ChannelOutput o;
	    getAnimChannelRawOutput(o);

	    if (getCurrentOutput() != o) {
	    	this->setOutput(o);
	    }
		break;
	}
	}
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

void RGBWWLed::blink(int time) {
	Serial.printf("Blink\n");
	if (_mode == ColorMode::Hsv) {
		HSVCT color = getCurrentColor();
		int val = (color.val) > 50 ? 0 : 100;
		_animChannelsHsv[CtrlChannel::Val]->pushAnimation(new AnimSetAndStay(val, time, this, CtrlChannel::Val), QueuePolicy::Front);
	}
	else {
		ChannelOutput out = getCurrentOutput();
		int cwVal = (out.coldwhite) > 50 ? 0 : 100;
		int wwVal = (out.warmwhite) > 50 ? 0 : 100;
		_animChannelsRaw[CtrlChannel::WarmWhite]->pushAnimation(new AnimSetAndStay(wwVal, time, this, CtrlChannel::WarmWhite), QueuePolicy::Front);
		_animChannelsRaw[CtrlChannel::ColdWhite]->pushAnimation(new AnimSetAndStay(cwVal, time, this, CtrlChannel::ColdWhite), QueuePolicy::Front);
	}
}

//// setHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::setHSV(const RequestHSVCT& color, QueuePolicy queuePolicy, bool requeue, const String& name) {
	setHSV(color, 0, queuePolicy, requeue, name);
}

void RGBWWLed::setHSV(const RequestHSVCT& color, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Hsv;

	pushAnimSetAndStay(color.h, time, queuePolicy, CtrlChannel::Hue, requeue, name);
	pushAnimSetAndStay(color.s, time, queuePolicy, CtrlChannel::Sat, requeue, name);
	pushAnimSetAndStay(color.v, time, queuePolicy, CtrlChannel::Val, requeue, name);
	pushAnimSetAndStay(color.ct, time, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
}

//// fadeHSV ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::fadeHSV(const RequestHSVCT& color, int ramp, int direction, bool requeue, const String& name) {
	fadeHSV( color, ramp, direction, QueuePolicy::Single, requeue, name);
}

void RGBWWLed::fadeHSV(const RequestHSVCT& color, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
	fadeHSV( color, ramp, 1, queuePolicy, requeue, name);
}

void RGBWWLed::fadeHSV(const RequestHSVCT& color, int ramp, int direction, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Hsv;

	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pushAnimSetAndStay(color.h, 0, queuePolicy, CtrlChannel::Hue, requeue, name);
		pushAnimSetAndStay(color.s, 0, queuePolicy, CtrlChannel::Sat, requeue, name);
		pushAnimSetAndStay(color.v, 0, queuePolicy, CtrlChannel::Val, requeue, name);
		pushAnimSetAndStay(color.ct, 0, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
	}
	else {
		pushAnimTransitionCircularHue(color.h, ramp, direction, queuePolicy, CtrlChannel::Hue, requeue, name);
		pushAnimTransition(color.s, ramp, queuePolicy, CtrlChannel::Sat, requeue, name);
		pushAnimTransition(color.v, ramp, queuePolicy, CtrlChannel::Val, requeue, name);
		pushAnimTransition(color.ct, ramp, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
	}
}

void RGBWWLed::fadeHSV(const RequestHSVCT& colorFrom, const RequestHSVCT& color, int ramp, int direction /* = 1 */, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Hsv;

	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pushAnimSetAndStay(color.h, 0, queuePolicy, CtrlChannel::Hue, requeue, name);
		pushAnimSetAndStay(color.s, 0, queuePolicy, CtrlChannel::Sat, requeue, name);
		pushAnimSetAndStay(color.v, 0, queuePolicy, CtrlChannel::Val, requeue, name);
		pushAnimSetAndStay(color.ct, 0, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
	}
	else {
		pushAnimTransitionCircularHue(colorFrom.h, color.h, ramp, direction, queuePolicy, CtrlChannel::Hue, requeue, name);
		pushAnimTransition(colorFrom.s, color.s, ramp, queuePolicy, CtrlChannel::Sat, requeue, name);
		pushAnimTransition(colorFrom.v, color.v, ramp, queuePolicy, CtrlChannel::Val, requeue, name);
		pushAnimTransition(colorFrom.ct, color.ct, ramp, queuePolicy, CtrlChannel::ColorTemp, requeue, name);
	}
}

//// setRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::setRAW(const RequestChannel& output, QueuePolicy queuePolicy, bool requeue, const String& name) {
	setRAW(output, 0, queuePolicy, requeue, name);
}

void RGBWWLed::setRAW(const RequestChannel& output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Raw;

	pushAnimSetAndStay(output.r, time, queuePolicy, CtrlChannel::Red, requeue, name);
	pushAnimSetAndStay(output.g, time, queuePolicy, CtrlChannel::Green, requeue, name);
	pushAnimSetAndStay(output.b, time, queuePolicy, CtrlChannel::Blue, requeue, name);
	pushAnimSetAndStay(output.cw, time, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
	pushAnimSetAndStay(output.ww, time, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
}

//// fadeRAW ////////////////////////////////////////////////////////////////////////////////////////////////////

void RGBWWLed::fadeRAW(const RequestChannel& output, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Raw;

	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pushAnimSetAndStay(output.r, 0, queuePolicy, CtrlChannel::Red, requeue, name);
		pushAnimSetAndStay(output.g, 0, queuePolicy, CtrlChannel::Green, requeue, name);
		pushAnimSetAndStay(output.b, 0, queuePolicy, CtrlChannel::Blue, requeue, name);
		pushAnimSetAndStay(output.ww, 0, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
		pushAnimSetAndStay(output.cw, 0, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
	}
	else {
		pushAnimTransition(output.r, ramp, queuePolicy, CtrlChannel::Red, requeue, name);
		pushAnimTransition(output.g, ramp, queuePolicy, CtrlChannel::Green, requeue, name);
		pushAnimTransition(output.b, ramp, queuePolicy, CtrlChannel::Blue, requeue, name);
		pushAnimTransition(output.ww, ramp, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
		pushAnimTransition(output.cw, ramp, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
	}
}

void RGBWWLed::fadeRAW(const RequestChannel& output_from, const RequestChannel& output, int ramp, QueuePolicy queuePolicy, bool requeue, const String& name) {
	_mode = ColorMode::Raw;

	if (ramp == 0 || ramp < RGBWW_MINTIMEDIFF) {
		pushAnimSetAndStay(output.r, 0, queuePolicy, CtrlChannel::Red, requeue, name);
		pushAnimSetAndStay(output.g, 0, queuePolicy, CtrlChannel::Green, requeue, name);
		pushAnimSetAndStay(output.b, 0, queuePolicy, CtrlChannel::Blue, requeue, name);
		pushAnimSetAndStay(output.ww, 0, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
		pushAnimSetAndStay(output.cw, 0, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
	}
	else {
		pushAnimTransition(output_from.r, output.r, ramp, queuePolicy, CtrlChannel::Red, requeue, name);
		pushAnimTransition(output_from.g, output.g, ramp, queuePolicy, CtrlChannel::Green, requeue, name);
		pushAnimTransition(output_from.b, output.b, ramp, queuePolicy, CtrlChannel::Blue, requeue, name);
		pushAnimTransition(output_from.ww, output.ww, ramp, queuePolicy, CtrlChannel::WarmWhite, requeue, name);
		pushAnimTransition(output_from.cw, output.cw, ramp, queuePolicy, CtrlChannel::ColdWhite, requeue, name);
	}
}

void RGBWWLed::pushAnimSetAndStay(const Optional<AbsOrRelValue>& val, int time, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name, const ChannelList& channels) {
	if (!val.hasValue())
		return;
	RGBWWLedAnimation* pAnim = new AnimSetAndStay(val, time, this, ch, requeue, name);
	dispatchAnimation(pAnim, ch, queuePolicy, channels);
}

void RGBWWLed::pushAnimTransition(const Optional<AbsOrRelValue>& val, int ramp, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name, const ChannelList& channels) {
	if (!val.hasValue())
		return;
	RGBWWLedAnimation* pAnim = new AnimTransition(val, ramp, this, ch, requeue, name);
	dispatchAnimation(pAnim, ch, queuePolicy, channels);
}

void RGBWWLed::pushAnimTransition(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, int ramp, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name, const ChannelList& channels) {
	if (!val.hasValue())
		return;
	RGBWWLedAnimation* pAnim = new AnimTransition(from, val, ramp, this, ch, requeue, name);
	dispatchAnimation(pAnim, ch, queuePolicy, channels);
}

void RGBWWLed::pushAnimTransitionCircularHue(const Optional<AbsOrRelValue>& val, int ramp, int direction, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name, const ChannelList& channels) {
	if (!val.hasValue())
		return;
	RGBWWLedAnimation* pAnim = new AnimTransitionCircularHue(val, ramp, direction, this, ch, requeue, name);
	dispatchAnimation(pAnim, ch, queuePolicy, channels);
}

void RGBWWLed::pushAnimTransitionCircularHue(const AbsOrRelValue& from, const Optional<AbsOrRelValue>& val, int ramp, int direction, QueuePolicy queuePolicy, CtrlChannel ch, bool requeue, const String& name, const ChannelList& channels) {
	if (!val.hasValue())
		return;
	RGBWWLedAnimation* pAnim = new AnimTransitionCircularHue(from, val, ramp, direction, this, ch, requeue, name);
	dispatchAnimation(pAnim, ch, queuePolicy, channels);
}


void RGBWWLed::dispatchAnimation(RGBWWLedAnimation* pAnim, CtrlChannel ch, QueuePolicy queuePolicy, const ChannelList& channels) {
	if (channels.count() != 0 && !channels.contains(ch))
		return;

	switch(ch) {
	case CtrlChannel::Hue:
	case CtrlChannel::Sat:
	case CtrlChannel::Val:
	case CtrlChannel::ColorTemp:
		_animChannelsHsv[ch]->pushAnimation(pAnim, queuePolicy);
		break;
	case CtrlChannel::Red:
	case CtrlChannel::Green:
	case CtrlChannel::Blue:
	case CtrlChannel::WarmWhite:
	case CtrlChannel::ColdWhite:
		_animChannelsRaw[ch]->pushAnimation(pAnim, queuePolicy);
		break;
	}
}


void RGBWWLed::clearAnimationQueue(const ChannelList& channels) {
	callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::clearAnimationQueue);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::clearAnimationQueue);
}

void RGBWWLed::skipAnimation(const ChannelList& channels) {
	callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::skipAnimation);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::skipAnimation);
}

void RGBWWLed::pauseAnimation(const ChannelList& channels) {
	callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::pauseAnimation);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::pauseAnimation);
}

void RGBWWLed::continueAnimation(const ChannelList& channels) {
    callForChannels(_animChannelsHsv, &RGBWWAnimatedChannel::continueAnimation);
    callForChannels(_animChannelsRaw, &RGBWWAnimatedChannel::continueAnimation);
}

void RGBWWLed::callForChannels(const ChannelGroup& group, void (RGBWWAnimatedChannel::*fnc)(), const ChannelList& channels) {
	const bool all = (channels.size() == 0);

    for(int i=0; i < group.count(); ++i) {
        if (!all || !channels.contains(group.keyAt(i)))
            continue;
        (group.valueAt(i)->*fnc)();
    }
}

void RGBWWLed::setAnimationCallback(void (*func)(RGBWWLed* led, RGBWWLedAnimation* anim)) {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->setAnimationCallback(func);
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->setAnimationCallback(func);
	}
}

///////
