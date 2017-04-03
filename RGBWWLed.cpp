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

	_pwm_output = NULL;

	_animChannelsHsv[CtrlChannel::Hue] = new RGBWWAnimatedChannel(this);
	_animChannelsHsv[CtrlChannel::Sat] = new RGBWWAnimatedChannel(this);
	_animChannelsHsv[CtrlChannel::Val] = new RGBWWAnimatedChannel(this);
	_animChannelsHsv[CtrlChannel::ColorTemp] = new RGBWWAnimatedChannel(this);
}

RGBWWLed::~RGBWWLed() {
	if (_pwm_output != NULL) {
		delete _pwm_output;
	}

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

/**************************************************************
 *                     OUTPUT
 **************************************************************/
bool RGBWWLed::show() {
	switch(_mode) {
	case ColorMode::Hsv:
	{
	    for(int i=0; i < _animChannelsHsv.count(); ++i) {
	        CtrlChannel ch = _animChannelsHsv.keyAt(i);
	        RGBWWAnimatedChannel* pAnimCh = _animChannelsHsv[ch];
	        bool result = pAnimCh->process();
	    }

	    // now build the output value and set
	    HSVCT c;
	    getAnimChannelHsvColor(c);

	    debugRGBW("NEW: h:%d, s:%d, v:%d, ct: %d", c.h, c.s, c.v, c.ct);

	    if (getCurrentColor() != c) {
			debugRGBW("DDDDDDDDDDDDDDDDDD");
	    	this->setOutput(c);
	    }
		break;
	}
	case ColorMode::Raw:
	{
	    for(int i=0; i < _animChannelsRaw.count(); ++i) {
	        CtrlChannel ch = _animChannelsRaw.keyAt(i);
	        RGBWWAnimatedChannel* pAnimCh = _animChannelsRaw[ch];
	        bool result = pAnimCh->process();
	    }

	    // now build the output value and set
	    ChannelOutput o;
	    o.r = _animChannelsRaw[CtrlChannel::Red]->getValue();
	    o.g = _animChannelsRaw[CtrlChannel::Green]->getValue();
	    o.b = _animChannelsRaw[CtrlChannel::Blue]->getValue();
	    o.cw = _animChannelsRaw[CtrlChannel::WarmWhite]->getValue();
	    o.ww = _animChannelsRaw[CtrlChannel::ColdWhite]->getValue();
	    this->setOutput(o);
		break;
	}
	}
}

void RGBWWLed::refresh() {
	setOutput(_current_color);
}

ChannelOutput RGBWWLed::getCurrentOutput() const {
	return _current_output;
}


HSVCT RGBWWLed::getCurrentColor() const {
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



void RGBWWLed::blink() {
	Serial.printf("Blink\n");
	if (_mode == ColorMode::Hsv) {
		HSVCT color = getCurrentColor();
		int val = (color.val) > 50 ? 0 : 100;
		//_animChannelsHsv[CtrlChannel::Val]->pushAnimation(new AnimSetAndStay(val, 1000, this, false, CtrlChannel::Val), QueuePolicy::Front);
	}
	else {
		ChannelOutput out = getCurrentOutput();
		//out.warmwhite = out.coldwhite = 100;
		//_animChannelsRaw[CtrlChannel::WarmWhite]->pushAnimation(new AnimSetAndStay(val, 1000, this, false, CtrlChannel::WarmWhite), QueuePolicy::Front);
	}
}


void RGBWWLed::setHSV(HSVCT& color, QueuePolicy queuePolicy, bool requeue, const String& name) {
//	pushAnimation(new HSVSetOutput(color, this), queuePolicy);
}

void RGBWWLed::setHSV(HSVCT& color, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
//	pushAnimation(new HSVSetOutput(color, this, time), queuePolicy);
}


void RGBWWLed::fadeHSV(HSVCT& color, int time,QueuePolicy queuePolicy, bool requeue, const String& name) {
	fadeHSV( color, time, 1, queuePolicy, requeue, name);
}


void RGBWWLed::fadeHSV(HSVCT& color, int time, int direction, bool requeue, const String& name) {
	fadeHSV( color, time, direction, QueuePolicy::Single, requeue, name);
}


void RGBWWLed::fadeHSV(HSVCT& color, int time, int direction /* = 1 */, QueuePolicy queuePolicy, bool requeue, const String& name) {
	RGBWWLedAnimation* pAnimH = NULL;
	RGBWWLedAnimation* pAnimS = NULL;
	RGBWWLedAnimation* pAnimV = NULL;
	RGBWWLedAnimation* pAnimCt = NULL;
	if (time == 0 || time < RGBWW_MINTIMEDIFF) {
		pAnimH = new AnimSetAndStay(color.h, 0, this, CtrlChannel::Hue, requeue, name);
		pAnimS = new AnimSetAndStay(color.s, 0, this, CtrlChannel::Sat, requeue, name);
		pAnimV = new AnimSetAndStay(color.v, 0, this, CtrlChannel::Val, requeue, name);
		pAnimCt = new AnimSetAndStay(color.ct, 0, this, CtrlChannel::ColorTemp, requeue, name);
	}
	else {
		pAnimH = new AnimTransitionCircularHue(color.h, time, direction, this, CtrlChannel::Hue, requeue, name);
		pAnimS = new AnimTransition(color.s, time, this, CtrlChannel::Sat, requeue, name);
		pAnimV = new AnimTransition(color.v, time, this, CtrlChannel::Val, requeue, name);
		pAnimCt = new AnimTransition(color.ct, time, this, CtrlChannel::ColorTemp, requeue, name);
	}

	_animChannelsHsv[CtrlChannel::Hue]->pushAnimation(pAnimH, queuePolicy);
	_animChannelsHsv[CtrlChannel::Sat]->pushAnimation(pAnimS, queuePolicy);
	_animChannelsHsv[CtrlChannel::Val]->pushAnimation(pAnimV, queuePolicy);
	_animChannelsHsv[CtrlChannel::ColorTemp]->pushAnimation(pAnimCt, queuePolicy);
}

void RGBWWLed::fadeHSV(HSVCT& colorFrom, HSVCT& color, int time, int direction /* = 1 */, QueuePolicy queuePolicy, bool requeue, const String& name) {
//	if (colorFrom == color)
//		return;
//
//	RGBWWLedAnimation* pAnim = NULL;
//	if (time == 0 || time < RGBWW_MINTIMEDIFF)
//		pAnim = new HSVSetOutput(color, this, requeue, name);
//	else
//		pAnim = new HSVTransition(colorFrom, color, time, direction, this, requeue, name);
//	pushAnimation(pAnim, queuePolicy);
}

void RGBWWLed::setRAW(ChannelOutput output, QueuePolicy queuePolicy, bool requeue, const String& name) {
	//pushAnimation(new RAWSetOutput(output, this, requeue, name), queuePolicy);
}

void RGBWWLed::setRAW(ChannelOutput output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
//	pushAnimation(new RAWSetOutput(output, this, time, requeue, name), queuePolicy);
}

void RGBWWLed::fadeRAW(ChannelOutput output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
//	RGBWWLedAnimation* pAnim = NULL;
//	if (time == 0 || time < RGBWW_MINTIMEDIFF)
//		pAnim = new RAWSetOutput(output, this, requeue, name);
//	else
//		pAnim = new RAWTransition(output, time, this, requeue, name);
//
//	pushAnimation(pAnim, queuePolicy);
}

void RGBWWLed::fadeRAW(ChannelOutput output_from, ChannelOutput output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {

}

void RGBWWLed::clearAnimationQueue() {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->clearAnimationQueue();
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->clearAnimationQueue();
	}
}

void RGBWWLed::skipAnimation() {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->skipAnimation();
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->skipAnimation();
	}
}

void RGBWWLed::pauseAnimation() {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->pauseAnimation();
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->pauseAnimation();
	}
}

void RGBWWLed::continueAnimation() {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->continueAnimation();
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->continueAnimation();
	}
}

#if 0
void RGBWWLed::fadeRAW(ChannelOutput output_from, ChannelOutput output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	if (output_from == output)
		return;

	RGBWWLedAnimation* pAnim = NULL;
	if (time == 0 || time < RGBWW_MINTIMEDIFF)
		pAnim = new RAWSetOutput(output, this, requeue, name);
	else
		pAnim = new RAWTransition(output_from, output, time, this, requeue, name);

	pushAnimation(pAnim, queuePolicy);
}
#endif

void RGBWWLed::setAnimationCallback( void (*func)(RGBWWLed* led, RGBWWLedAnimation* anim) ) {
	for(int i=0; i < _animChannelsHsv.count(); ++i) {
		_animChannelsHsv.valueAt(i)->setAnimationCallback(func);
	}
	for(int i=0; i < _animChannelsRaw.count(); ++i) {
		_animChannelsRaw.valueAt(i)->setAnimationCallback(func);
	}
}
