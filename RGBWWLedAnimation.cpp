/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite strips
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#include "RGBWWLedAnimation.h"
#include "RGBWWLed.h"
#include "RGBWWLedColor.h"

RGBWWLedAnimation::RGBWWLedAnimation(RGBWWLed const * rgbled, CtrlChannel ch, bool requeue, const String& name) : _rgbled(rgbled),
																													_ctrlChannel(ch),
																													_requeue(requeue),
																													_name(name) {
}


int RGBWWLedAnimation::getBaseValue() const {
	const HSVCT& c = _rgbled->getCurrentColor();
	const ChannelOutput& o = _rgbled->getCurrentOutput();

	Serial.printf("CTRL: %d\n", _ctrlChannel);
	switch(_ctrlChannel) {
	case CtrlChannel::Hue:
		Serial.printf("HUE:BASE: %d\n", c.hue);
		return c.hue;
		break;
	case CtrlChannel::Sat:
		return c.sat;
		break;
	case CtrlChannel::Val:
		return c.val;
		break;
	case CtrlChannel::ColorTemp:
		return c.ct;
		break;

	case CtrlChannel::Red:
		return o.red;
		break;
	case CtrlChannel::Green:
		return o.green;
		break;
	case CtrlChannel::Blue:
		return o.blue;
		break;
	case CtrlChannel::ColdWhite:
		return o.coldwhite;
		break;
	case CtrlChannel::WarmWhite:
		return o.warmwhite;
		break;
	}
}

AnimSetAndStay::AnimSetAndStay(const AbsOrRelValue& endVal,
									int time,
									RGBWWLed const * rgbled,
									CtrlChannel ch,
									bool requeue,
									const String& name) : RGBWWLedAnimation(rgbled, ch, requeue, name),
															_endVal(enVal),
															_time(time) {
}

bool AnimSetAndStay::run() {
	if (_currentstep == 0)
		init();

    _currentstep += 1;
    if (_steps != 0) {
        if (_currentstep < _steps) {
            return false;
        }
    }

    return true;
}

bool AnimSetAndStay::init() {
	_value = _endVal.getFinalValue(getBaseValue());

	if (time > 0) {
        _steps = time / RGBWW_MINTIMEDIFF;
    }
}

void AnimSetAndStay::reset() {
	_currentstep = 0;
}

AnimTransition::AnimTransition(const AbsOrRelValue& endVal,
								int ramp,
								RGBWWLed const * rgbled,
								CtrlChannel ch,
								bool requeue,
								const String& name) : RGBWWLedAnimation(rgbled, ch, requeue, name)
								{
	_endVal = endVal;
    _steps = ramp / RGBWW_MINTIMEDIFF;
}

AnimTransition::AnimTransition(int startVal,
								const AbsOrRelValue& endVal,
								int ramp,
								RGBWWLed const * rgbled,
								CtrlChannel ch,
								bool requeue,
								const String& name) : AnimTransition(endVal, ramp, rgbled, ch, requeue, name) {
	_baseval = startVal;
	_hasbaseval = true;
}

bool AnimTransition::init() {
	_finalval = _endVal.getFinalValue(getBaseValue());

    int l, r, d;
    if (!_hasbaseval) {
    	_baseval = _baseval.getFinalValue(getBaseValue());
    	Serial.printf("AnimTransition::init: %d\n", _baseval);
    }
    _value = _baseval;

    //calculate steps per time
    _steps = (_steps > 0) ? _steps : int(1); //avoid 0 division

    _bresenham.delta = abs(_baseval - _finalval);
    _bresenham.step = 1;
    _bresenham.step = (_bresenham.delta < _steps) ? (_bresenham.step << 8) : (_bresenham.delta << 8)/_steps;
    _bresenham.step = (_baseval > _finalval) ? _bresenham.step*=-1 : _bresenham.step;
    _bresenham.error = -1*_steps;
    _bresenham.count = 0;

    return true;
}

bool AnimTransition::run () {
    if (_currentstep == 0) {
        if (!init()) {
            return true;
        }
        _currentstep = 0;
    }

    _currentstep++;
    if (_currentstep >= _steps) {
        // ensure that the with the last step
        // we arrive at the destination color
        _value = _finalval;
        return true;
    }

    //calculate new colors with bresenham
    _value = bresenham(_bresenham, _steps, _baseval, _value);

    return false;
}

void AnimTransition::reset() {
	_currentstep = 0;
}

int AnimTransition::bresenham(BresenhamValues& values, int& dx, int& base, int& current) {
    //more information on bresenham:
    //https://www.cs.helsinki.fi/group/goa/mallinnus/lines/bresenh.html
    values.error = values.error + 2 * values.delta;
    if (values.error > 0) {
        values.count += 1;
        values.error = values.error - 2*dx;
        return base + ((values.count * values.step) >> 8);
    }
    return current;
}


///////////////////////////////

AnimTransitionCircularHue::AnimTransitionCircularHue(const AbsOrRelValue& endVal,
														int ramp,
														int direction,
														RGBWWLed const * rgbled,
														CtrlChannel ch,
														bool requeue,
														const String& name) : AnimTransition(endVal, ramp, rgbled, ch, requeue, name),
																_direction(direction) {
}

AnimTransitionCircularHue::AnimTransitionCircularHue(const AbsOrRelValue& startVal,
														const AbsOrRelValue& endVal,
														int ramp,
														int direction,
														RGBWWLed const * rgbled,
														CtrlChannel ch,
														bool requeue,
														const String& name) : AnimTransition(startVal, endVal, ramp, rgbled, ch, requeue, name)
																{
	_direction = direction;
}

bool AnimTransitionCircularHue::init() {
	Serial.printf("HAS_BASEVAL: %d\n", _hasbaseval);
	_finalval = _endVal.getFinalValue(getBaseValue());
	if (!_hasbaseval) {
		Serial.printf("GETBASEVAL\n");
		_baseval = getBaseValue();
	}
	Serial.printf("BASEVAL: %d\n", _baseval);
	_value = _baseval;

	// calculate hue direction
	const int l = (_baseval + RGBWW_CALC_HUEWHEELMAX - _finalval) % RGBWW_CALC_HUEWHEELMAX;
	const int r = (_finalval + RGBWW_CALC_HUEWHEELMAX - _baseval) % RGBWW_CALC_HUEWHEELMAX;

	// decide on direction of turn depending on size
	int d = (l < r) ? -1 : 1;

	// turn direction if user wishes for long transition
	d = (_direction == 1) ? d : d *= -1;

	//calculate steps per time
	_steps = (_steps > 0) ? _steps : int(1); //avoid 0 division

	//HUE
	_bresenham.delta = (d == -1) ? l : r;
	_bresenham.step = 1;
	_bresenham.step = (_bresenham.delta < _steps) ? (_bresenham.step <<8) : (_bresenham.delta << 8)/_steps;
	_bresenham.step *= d;
	_bresenham.error = -1 * _steps;
	_bresenham.count = 0;

	return true;
}

bool AnimTransitionCircularHue::run() {
	const bool result = AnimTransition::run();
	RGBWWColorUtils::circleHue(_value);
	return result;
}

/**************************************************************
 *               HSVSetOutput
 **************************************************************/
#if 0

HSVSetOutput::HSVSetOutput(const HSVCT& color, RGBWWLed* ctrl, int time /* = 0 */, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    outputcolor = color;
    rgbwwctrl = ctrl;
    steps = 0;
    if (time > 0) {
        steps = time / RGBWW_MINTIMEDIFF;
    }
    count = 0;
}

bool HSVSetOutput::run() {
    if (count == 0) {
        rgbwwctrl->setOutput(outputcolor);
    }
    count += 1;
    if (steps != 0) {
        if (count < steps) {
            return false;
        }
    }

    return true;
}


/**************************************************************
 *               HSV Transition
 **************************************************************/


HSVTransition::HSVTransition(const HSVCT& colorEnd, const int& time, const int& direction, RGBWWLed* ctrl, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    rgbwwctrl = ctrl;
    _finalcolor = colorEnd;
    _hasbasecolor = false;
    _steps = time / RGBWW_MINTIMEDIFF;
    _huedirection = direction;
    _currentstep = 0;
}


HSVTransition::HSVTransition(const HSVCT& colorFrom, const HSVCT& colorEnd, const int& time, const int& direction, RGBWWLed* ctrl, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    rgbwwctrl = ctrl;
    _finalcolor = colorEnd;
    _basecolor = colorFrom;
    _hasbasecolor = true;
    _steps = time / RGBWW_MINTIMEDIFF;
    _huedirection = direction;
    _currentstep = 0;

}

bool HSVTransition::init() {
    int l, r, d;
    if (!_hasbasecolor) {
        _basecolor = rgbwwctrl->getCurrentColor();
    }
    _currentcolor = _basecolor;

    // calculate hue direction
    l = (_basecolor.h + RGBWW_CALC_HUEWHEELMAX - _finalcolor.h) % RGBWW_CALC_HUEWHEELMAX;
    r = (_finalcolor.h + RGBWW_CALC_HUEWHEELMAX - _basecolor.h) % RGBWW_CALC_HUEWHEELMAX;

    // decide on direction of turn depending on size
    d = (l < r) ? -1 : 1;

    // turn direction if user wishes for long transition
    d = (_huedirection == 1) ? d : d *= -1;

    //calculate steps per time
    _steps = (_steps > 0) ? _steps : int(1); //avoid 0 division


    //HUE
    hue.delta = (d == -1) ? l : r;
    hue.step = 1;
    hue.step = (hue.delta < _steps) ? (hue.step <<8) : (hue.delta << 8)/_steps;
    hue.step *= d;
    hue.error = -1 * _steps;
    hue.count = 0;

    //SATURATION
    sat.delta = abs(_basecolor.s - _finalcolor.s);
    sat.step = 1;
    sat.step = (sat.delta < _steps) ? (sat.step << 8) : (sat.delta << 8)/_steps;
    sat.step = (_basecolor.s > _finalcolor.s) ? sat.step*=-1 : sat.step;
    sat.error = -1* _steps;
    sat.count = 0;

    //VALUE
    val.delta = abs(_basecolor.v - _finalcolor.v);
    val.step = 1;
    val.step = (val.delta < _steps) ? (val.step << 8) : (val.delta << 8)/_steps;
    val.step = (_basecolor.v > _finalcolor.v) ? val.step*=-1 : val.step;
    val.error = -1*_steps;
    val.count = 0;

    //KELVIN
    ct.delta = abs(_basecolor.ct - _finalcolor.ct);
    ct.step = 1;
    ct.step = (ct.delta < _steps) ? (ct.step << 8 ): (ct.delta << 8)/_steps;
    ct.step = (_basecolor.ct > _finalcolor.ct) ? ct.step*=-1 : ct.step;
    ct.error = -1 * _steps;
    ct.count = 0;
    return true;
}

bool HSVTransition::run () {

    if (_currentstep == 0) {
        if (!init()) {
            return true;
        }
        _currentstep = 0;
    }
    debugRGBW("HSVTransition::run CURRENT  H %i | S %i | V %i | K %i", _currentcolor.h, _currentcolor.s, _currentcolor.v, _currentcolor.ct);
    debugRGBW("HSVTransition::run FINAL    H %i | S %i | V %i | K %i", _finalcolor.h, _finalcolor.s, _finalcolor.v, _finalcolor.ct);
    _currentstep++;
    if (_currentstep >= _steps) {
        // ensure that the with the last step
        // we arrive at the destination color
        rgbwwctrl->setOutput(_finalcolor);
        return true;
    }
    rgbwwctrl->setOutput(_currentcolor);

    //calculate new colors with bresenham
    _currentcolor.h = bresenham(hue, _steps, _basecolor.h, _currentcolor.h);
    _currentcolor.s = bresenham(sat, _steps, _basecolor.s, _currentcolor.s);
    _currentcolor.v = bresenham(val, _steps,_basecolor.v, _currentcolor.v);
    _currentcolor.ct = bresenham(ct, _steps, _basecolor.ct, _currentcolor.ct);

    //fix hue
    RGBWWColorUtils::circleHue(_currentcolor.h);


    return false;
}

void HSVTransition::reset() {
    _currentstep = 0;
}

int HSVTransition::bresenham(BresenhamValues& values, int& dx, int& base, int& current) {
    //more information on bresenham:
    //https://www.cs.helsinki.fi/group/goa/mallinnus/lines/bresenh.html
    values.error = values.error + 2 * values.delta;
    if (values.error > 0) {
        values.count += 1;
        values.error = values.error - 2*dx;
        return base + ((values.count * values.step) >> 8);
    }
    return current;
}

/**************************************************************
 *               RAWSetOutput
 **************************************************************/


RAWSetOutput::RAWSetOutput(const ChannelOutput& output, RGBWWLed* ctrl, int time /* = 0 */, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    outputcolor = output;
    rgbwwctrl = ctrl;
    steps = 0;
    if (time > 0) {
        steps = time / RGBWW_MINTIMEDIFF;
    }
    count = 0;
}

bool RAWSetOutput::run() {
    if (count == 0) {
        rgbwwctrl->setOutput(outputcolor);
    }
    count += 1;
    if (steps != 0) {
        if (count < steps) {
            return false;
        }
    }
    return true;
}


/**************************************************************
 *               RAW Transition
 **************************************************************/


RAWTransition::RAWTransition(const ChannelOutput& output, const int& time, RGBWWLed* ctrl, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    rgbwwctrl = ctrl;
    _finalcolor = output;
    _hasbasecolor = false;
    _steps = time / RGBWW_MINTIMEDIFF;
    _currentstep = 0;
}


RAWTransition::RAWTransition(const ChannelOutput& output_from, const ChannelOutput& output, const int& time, RGBWWLed* ctrl, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    rgbwwctrl = ctrl;
    _finalcolor = output;
    _basecolor = output_from;
    _hasbasecolor = true;
    _steps = time / RGBWW_MINTIMEDIFF;
    _currentstep = 0;

}

bool RAWTransition::init() {
    if (!_hasbasecolor) {
        _basecolor = rgbwwctrl->getCurrentOutput();
    }

    _currentcolor = _basecolor;

    // calculate steps per time
    _steps = (_steps > 0) ? _steps : int(1); //avoid 0 division


    // RED
    red.delta = abs(_basecolor.r - _finalcolor.r);
    red.step = 1;
    red.step = (red.delta < _steps) ? (red.step << 8) : (red.delta << 8)/_steps;
    red.step = (_basecolor.r > _finalcolor.r) ? red.step*=-1 : red.step;
    red.error = -1* _steps;
    red.count = 0;

    // GREEN
    green.delta = abs(_basecolor.g - _finalcolor.g);
    green.step = 1;
    green.step = (green.delta < _steps) ? (green.step << 8) : (green.delta << 8)/_steps;
    green.step = (_basecolor.g > _finalcolor.g) ? green.step*=-1 : green.step;
    green.error = -1* _steps;
    green.count = 0;

    // BLUE
    blue.delta = abs(_basecolor.b - _finalcolor.b);
    blue.step = 1;
    blue.step = (blue.delta < _steps) ? (blue.step << 8) : (blue.delta << 8)/_steps;
    blue.step = (_basecolor.b > _finalcolor.b) ? blue.step*=-1 : blue.step;
    blue.error = -1* _steps;
    blue.count = 0;

    // WW
    warmwhite.delta = abs(_basecolor.ww - _finalcolor.ww);
    warmwhite.step = 1;
    warmwhite.step = (warmwhite.delta < _steps) ? (warmwhite.step << 8) : (warmwhite.delta << 8)/_steps;
    warmwhite.step = (_basecolor.ww > _finalcolor.ww) ? warmwhite.step*=-1 : warmwhite.step;
    warmwhite.error = -1* _steps;
    warmwhite.count = 0;

    // CW
    coldwhite.delta = abs(_basecolor.cw - _finalcolor.cw);
    coldwhite.step = 1;
    coldwhite.step = (coldwhite.delta < _steps) ? (coldwhite.step << 8) : (coldwhite.delta << 8)/_steps;
    coldwhite.step = (_basecolor.cw > _finalcolor.cw) ? coldwhite.step*=-1 : coldwhite.step;
    coldwhite.error = -1* _steps;
    coldwhite.count = 0;
    return true;
}



bool RAWTransition::run () {

    if (_currentstep == 0) {
        if (!init()) {
            return true;
        }
        _currentstep = 1;
    }
    debugRGBW("RAWTransition::run CURRENT  R %i | G %i | B %i | WW %i | CW %i  ", _currentcolor.r, _currentcolor.g, _currentcolor.b, _currentcolor.ww, _currentcolor.cw);
    debugRGBW("RAWTransition::run FINAL    R %i | G %i | B %i | WW %i | CW %i ", _finalcolor.r, _finalcolor.g, _finalcolor.b, _finalcolor.ww, _finalcolor.cw);


    if (_currentstep >= _steps) {
        // ensure that the with the last step
        // we arrive at the destination color
        rgbwwctrl->setOutput(_finalcolor);
        return true;
    }
    rgbwwctrl->setOutput(_currentcolor);
    _currentstep++;
    //calculate new colors with bresenham
    _currentcolor.r = bresenham(red, _steps, _basecolor.r, _currentcolor.r);
    _currentcolor.g = bresenham(green, _steps, _basecolor.g, _currentcolor.g);
    _currentcolor.b = bresenham(blue, _steps,_basecolor.b, _currentcolor.b);
    _currentcolor.ww = bresenham(warmwhite, _steps, _basecolor.ww, _currentcolor.ww);
    _currentcolor.cw = bresenham(coldwhite, _steps, _basecolor.cw, _currentcolor.cw);

    return false;
}

void RAWTransition::reset() {
    _currentstep = 0;
}


int RAWTransition::bresenham(BresenhamValues& values, int& dx, int& base, int& current) {
    //more information on bresenham:
    //https://www.cs.helsinki.fi/group/goa/mallinnus/lines/bresenh.html
    values.error = values.error + 2 * values.delta;
    if (values.error > 0) {
        values.count += 1;
        values.error = values.error - 2*dx;
        return base + ((values.count * values.step) >> 8);
    }
    return current;
}

/**************************************************************
                Animation Set
 **************************************************************/

RGBWWAnimationSet::RGBWWAnimationSet(RGBWWLedAnimation** animations, int count, bool loop /* =false */, bool requeue, const String& name) : RGBWWLedAnimation(requeue, name) {
    q = animations;
    _currentstep = count;
    _loop = loop;
}


RGBWWAnimationSet::~RGBWWAnimationSet(){
    for (int i = 0; i < _currentstep; i++) {
        delete q[i];
    }
}

void RGBWWAnimationSet::setSpeed(int newspeed) {
    _speed = newspeed;
    q[_current]->setSpeed(_speed);
};

void RGBWWAnimationSet::setBrightness(int newbrightness){
    _brightness = newbrightness;
    q[_current]->setBrightness(_brightness);
};

bool RGBWWAnimationSet::run(){

    if (q[_current]->run()) {
        q[_current]->reset();
        _current+=1;
        if (_brightness != -1) {
            q[_current]->setBrightness(_brightness);
        }
        if (_speed != -1) {
            q[_current]->setSpeed(_speed);
        }
        if (_current >= _currentstep) {
            if(_loop) {
                _current = 0;
            } else {
                return true; // finished set
            }
        }
    }

    return false; //continuing animation
};
#endif
