/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#include "RGBWWLed.h"
#include "RGBWWLedColor.h"
#include "RGBWWLedAnimation.h"
#include "RGBWWLedAnimationQ.h"
#include "RGBWWLedOutput.h"



/**************************************************************
 *                setup, init and settings
 **************************************************************/

RGBWWLed::RGBWWLed() {
	_isAnimationActive = false;
	_isAnimationPaused = false;
	_cancelAnimation = false;
	_clearAnimationQueue = false;
	_current_color = HSVCT(0, 0, 0);
	_current_output = ChannelOutput(0, 0, 0, 0, 0);
	_currentAnimation = NULL;
	_animationQ = new RGBWWLedAnimationQ(RGBWW_ANIMATIONQSIZE);
	_pwm_output = NULL;

	last_active = 0;

}

RGBWWLed::~RGBWWLed() {
	delete _animationQ;
	if (_currentAnimation != NULL) {
		delete _currentAnimation;
	}
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


bool RGBWWLed::show() {
	if (_isAnimationPaused) {
		return false;
	}

	// check if we need to cancel effect
	if (_cancelAnimation) {
		cleanupCurrentAnimation();
	}

	// cleanup Q if we cancel all effects
	if (_clearAnimationQueue) {
		cleanupAnimationQ();
	}

	// Interval has passed
	// check if we need to animate or there is any new animation
	if (!_isAnimationActive) {
		//check if animation otherwise return true
		if (_animationQ->isEmpty()) {
			return true;
		}
		_currentAnimation = _animationQ->pop();
		_isAnimationActive = true;
	}

	if (_currentAnimation->run()) {
		//callback animation finished
		if(_animationcallback != NULL ){
			_animationcallback(this, _currentAnimation);
		}

		if (_currentAnimation->shouldRequeue())
		    requeueCurrentAnimation();
		else
		    cleanupCurrentAnimation();
	}

	return false;

}

bool RGBWWLed::addToQueue(RGBWWLedAnimation* animation) {
	return _animationQ->push(animation);
}

void RGBWWLed::pauseAnimation() {
	_isAnimationPaused = true;
}

void RGBWWLed::continueAnimation() {
	_isAnimationPaused = false;
}

bool RGBWWLed::isAnimationQFull() {
	return _animationQ->isFull();
}


bool RGBWWLed::isAnimationActive() {
	return _isAnimationActive;
}


void RGBWWLed::skipAnimation(){
	if (_isAnimationActive) {
		_cancelAnimation = true;
	}
}

void RGBWWLed::clearAnimationQueue() {
	_clearAnimationQueue = true;
}


void RGBWWLed::setAnimationCallback( void (*func)(RGBWWLed* led, RGBWWLedAnimation* anim) ) {
  _animationcallback = func;
}

void RGBWWLed::setAnimationSpeed(int speed) {
	if(_currentAnimation != NULL) {
		_currentAnimation->setSpeed(speed);
	}
}

void RGBWWLed::setAnimationBrightness(int brightness){
	if(_currentAnimation != NULL) {
		_currentAnimation->setBrightness(brightness);
	}
}

void RGBWWLed::pushAnimation(RGBWWLedAnimation* pAnim, QueuePolicy queuePolicy) {
	if (queuePolicy == QueuePolicy::Single) {
		cleanupAnimationQ();
		cleanupCurrentAnimation();
	}

	Serial.printf("Adding animation: %d\n", queuePolicy);

	switch(queuePolicy) {
		case QueuePolicy::Back:
		case QueuePolicy::Single:
			_animationQ->push(pAnim);
			break;
		case QueuePolicy::Front:
		case QueuePolicy::FrontReset:
			if (_currentAnimation != nullptr) {
				if (queuePolicy == QueuePolicy::FrontReset)
					_currentAnimation->reset();
				_animationQ->pushFront(_currentAnimation);
				_currentAnimation = NULL;
			}
			_animationQ->pushFront(pAnim);
		    _isAnimationActive = false;
		    _cancelAnimation = false;
			break;
	}
}

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

void RGBWWLed::cleanupCurrentAnimation() {
	if (_currentAnimation == nullptr)
		return;

	_isAnimationActive = false;
	delete _currentAnimation;
	_currentAnimation = NULL;
	_cancelAnimation = false;
}

void RGBWWLed::cleanupAnimationQ() {
	_animationQ->clear();
	_clearAnimationQueue = false;
}

void RGBWWLed::requeueCurrentAnimation() {
    if (_currentAnimation == nullptr)
        return;

    Serial.printf("Requeuing...\n");

    _currentAnimation->reset();
    _animationQ->push(_currentAnimation);

    _currentAnimation = NULL;
    _isAnimationActive = false;
    _cancelAnimation = false;
}

