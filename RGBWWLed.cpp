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

	_animChannels[CtrlChannel::Hue] = new RGBWWAnimatedChannel();
    _animChannels[CtrlChannel::Sat] = new RGBWWAnimatedChannel();
    _animChannels[CtrlChannel::Val] = new RGBWWAnimatedChannel();
    _animChannels[CtrlChannel::ColorTemp] = new RGBWWAnimatedChannel();
}

RGBWWLed::~RGBWWLed() {
	if (_pwm_output != NULL) {
		delete _pwm_output;
	}

}

void RGBWWLed::init(int redPIN, int greenPIN, int bluePIN, int wwPIN, int cwPIN, int pwmFrequency /* =200 */) {
	_pwm_output = new PWMOutput(redPIN, greenPIN, bluePIN, wwPIN, cwPIN, pwmFrequency);
}



/**************************************************************
 *                     OUTPUT
 **************************************************************/
bool RGBWWLed::show() {
	switch(_mode) {
	case ColorMode::Hsv:
	    for(int i=0; i < _animChannelsHsv.count(); ++i) {
	        CtrlChannel ch = _animChannels.keyAt(i);
	        RGBWWAnimatedChannel* pAnimCh = _animChannelsHsv[ch];
	        bool result = pAnimCh->process();
	    }

	    // now build the output value and set
	    HSVCT c;
	    c.hue = _animChannelsHsv[CtrlChannel::Hue]->getValue();
	    c.sat = _animChannelsHsv[CtrlChannel::Sat]->getValue();
	    c.val = _animChannelsHsv[CtrlChannel::Val]->getValue();
	    c.ct = _animChannelsHsv[CtrlChannel::ColorTemp]->getValue();
	    this->setOutput(c);
		break;
	case ColorMode::Raw:
	    for(int i=0; i < _animChannelsRaw.count(); ++i) {
	        CtrlChannel ch = _animChannels.keyAt(i);
	        RGBWWAnimatedChannel* pAnimCh = _animChannelsRaw[ch];
	        bool result = pAnimCh->process();
	    }

	    // now build the output value and set
	    ChannelOutput c;
	    c.r = _animChannelsRaw[CtrlChannel::Red]->getValue();
	    c.g = _animChannelsRaw[CtrlChannel::Green]->getValue();
	    c.b = _animChannelsRaw[CtrlChannel::Blue]->getValue();
	    c.cw = _animChannelsRaw[CtrlChannel::WarmWhite]->getValue();
	    c.ww = _animChannelsRaw[CtrlChannel::ColdWhite]->getValue();
	    this->setOutput(c);
		break;
	}
}

void RGBWWLed::refresh() {
	setOutput(_current_color);
}

ChannelOutput RGBWWLed::getCurrentOutput() {
	return _current_output;
}


HSVCT RGBWWLed::getCurrentColor() {
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
	HSVCT color = getCurrentColor();
	color.val = (color.val) > 50 ? 0 : 100;
	pushAnimation(new HSVSetOutput(color, this, 1000), QueuePolicy::Front);
}


void RGBWWLed::setHSV(HSVCT& color, QueuePolicy queuePolicy, bool requeue, const String& name) {
	pushAnimation(new HSVSetOutput(color, this), queuePolicy);
}

void RGBWWLed::setHSV(HSVCT& color, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	pushAnimation(new HSVSetOutput(color, this, time), queuePolicy);
}


void RGBWWLed::fadeHSV(HSVCT& color, int time,QueuePolicy queuePolicy, bool requeue, const String& name) {
	fadeHSV( color, time, 1, queuePolicy, requeue, name);
}


void RGBWWLed::fadeHSV(HSVCT& color, int time, int direction, bool requeue, const String& name) {
	fadeHSV( color, time, direction, QueuePolicy::Single, requeue, name);
}


void RGBWWLed::fadeHSV(HSVCT& color, int time, int direction /* = 1 */, QueuePolicy queuePolicy, bool requeue, const String& name) {
	RGBWWLedAnimation* pAnim = NULL;
	if (time == 0 || time < RGBWW_MINTIMEDIFF)
		pAnim = new HSVSetOutput(color, this, requeue, name);
	else
		pAnim = new HSVTransition(color, time, direction, this, requeue, name);

	pushAnimation(pAnim, queuePolicy);
}


void RGBWWLed::fadeHSV(HSVCT& colorFrom, HSVCT& color, int time, int direction /* = 1 */, QueuePolicy queuePolicy, bool requeue, const String& name) {
	if (colorFrom == color)
		return;

	RGBWWLedAnimation* pAnim = NULL;
	if (time == 0 || time < RGBWW_MINTIMEDIFF)
		pAnim = new HSVSetOutput(color, this, requeue, name);
	else
		pAnim = new HSVTransition(colorFrom, color, time, direction, this, requeue, name);
	pushAnimation(pAnim, queuePolicy);
}

void RGBWWLed::setRAW(ChannelOutput output, QueuePolicy queuePolicy, bool requeue, const String& name) {
	pushAnimation(new RAWSetOutput(output, this, requeue, name), queuePolicy);
}

void RGBWWLed::setRAW(ChannelOutput output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	pushAnimation(new RAWSetOutput(output, this, time, requeue, name), queuePolicy);
}

void RGBWWLed::fadeRAW(ChannelOutput output, int time, QueuePolicy queuePolicy, bool requeue, const String& name) {
	RGBWWLedAnimation* pAnim = NULL;
	if (time == 0 || time < RGBWW_MINTIMEDIFF)
		pAnim = new RAWSetOutput(output, this, requeue, name);
	else
		pAnim = new RAWTransition(output, time, this, requeue, name);

	pushAnimation(pAnim, queuePolicy);
}


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

