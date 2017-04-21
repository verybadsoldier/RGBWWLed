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

    switch(_ctrlChannel) {
    case CtrlChannel::Hue:
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
        int onTime,
        RGBWWLed const * rgbled,
        CtrlChannel ch,
        bool requeue,
        const String& name) : RGBWWLedAnimation(rgbled, ch, requeue, name),
                _initEndVal(endVal) {
    if (onTime > 0) {
        _steps = onTime / RGBWW_MINTIMEDIFF;
    }
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
    _value = _initEndVal.getFinalValue(getBaseValue());
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
    _initEndVal = endVal;
    _steps = ramp / RGBWW_MINTIMEDIFF;
}

AnimTransition::AnimTransition(const AbsOrRelValue& from,
        const AbsOrRelValue& endVal,
        int ramp,
        RGBWWLed const * rgbled,
        CtrlChannel ch,
        bool requeue,
        const String& name) : AnimTransition(endVal, ramp, rgbled, ch, requeue, name) {
    _initStartVal = from;
    _hasfromval = true;
}

bool AnimTransition::init() {
    _finalval = _initEndVal.getFinalValue(getBaseValue());
    _baseval = _hasfromval ? _initStartVal.getFinalValue(getBaseValue()) : getBaseValue();

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
    _finalval = _initEndVal.getFinalValue(getBaseValue());
    _baseval = _hasfromval ? _initStartVal.getFinalValue(getBaseValue()) : getBaseValue();

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

AnimBlink::AnimBlink(int blinkTime,
        RGBWWLed const * rgbled,
        CtrlChannel ch,
        bool requeue,
        const String& name) : RGBWWLedAnimation(rgbled, ch, requeue, name) {
	if (blinkTime > 0) {
	    _steps = blinkTime / RGBWW_MINTIMEDIFF;
	}
}

bool AnimBlink::run() {
    if (_currentstep == 0)
        init();

    _currentstep += 1;
    if (_steps != 0) {
        if (_currentstep < _steps) {
            return false;
        }
    }

    // reset to the value before the blink
    _value = _prevvalue;

    return true;
}

bool AnimBlink::init() {
	// preserve the value before the blink
	_prevvalue = getBaseValue();

	switch(_ctrlChannel) {
	case CtrlChannel::Hue:
		_value += 180;
		RGBWWColorUtils::circleHue(_value);
		break;
	case CtrlChannel::Sat:
	case CtrlChannel::Val:
	    _value = (_prevvalue > 50) ? 0 : 100;
		break;
	case CtrlChannel::Red:
	case CtrlChannel::Green:
	case CtrlChannel::Blue:
	case CtrlChannel::ColdWhite:
	case CtrlChannel::WarmWhite:
	    _value = (_prevvalue > 512) ? 0 : 1023;
		break;
	}

}

void AnimBlink::reset() {
    _currentstep = 0;
}
