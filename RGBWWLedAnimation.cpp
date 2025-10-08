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

RGBWWLedAnimation::RGBWWLedAnimation(RGBWWLed const * rgbled, CtrlChannel ch, Type type, bool requeue, const String& name) :
        _rgbled(rgbled), _ctrlChannel(ch), _requeue(requeue), _name(name), _type(type) {
}

int RGBWWLedAnimation::getBaseValue() const {
    const HSVCT& c = _rgbled->getCurrentColor();
    const ChannelOutput& o = _rgbled->getCurrentOutput();

    switch (_ctrlChannel) {
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
    default:
        return 0;
    }
}

AnimTransition::AnimTransition(const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int stay, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue,
        const String& name) :
        RGBWWLedAnimation(rgbled, ch, Type::Transition, requeue, name), _stay(stay) {
    _initEndVal = endVal;
    _ramp = ramp;
}

AnimTransition::AnimTransition(const AbsOrRelValue& from, const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int stay, RGBWWLed const * rgbled, CtrlChannel ch,
        bool requeue, const String& name) :
        AnimTransition(endVal, ramp, stay, rgbled, ch, requeue, name) {
    _initStartVal = from;
    _hasfromval = true;
    _type = Type::Transition;
}

bool AnimTransition::init() {
    _finalval = _initEndVal.getFinalValue(getBaseValue());
    _baseval = _hasfromval ? _initStartVal.getFinalValue(getBaseValue()) : getBaseValue();

    _value = _baseval;

    switch (_ramp.type) {
    case RampTimeOrSpeed::Type::Time: {
        _stepsNeededFade = static_cast<int>(_ramp.value / RGBWW_MINTIMEDIFF);
        break;
    }
    case RampTimeOrSpeed::Type::Speed: {
        const double diffPerc = (abs(_finalval - _baseval) / static_cast<double>(RGBWW_CALC_MAXVAL)) * 100;
        _stepsNeededFade = static_cast<int>((diffPerc / _ramp.value) * 60 * 1000 + 0.5 / RGBWW_MINTIMEDIFF);
        break;
    }
    }

    _stepsNeededFade = max(_stepsNeededFade, 1); //avoid 0 division

    _bresenham.delta = abs(_baseval - _finalval);
    _bresenham.step = 1;
    _bresenham.step = (_bresenham.delta < _stepsNeededFade) ? (_bresenham.step << 8) : (_bresenham.delta << 8) / _stepsNeededFade;
    _bresenham.step *= (_baseval > _finalval) ? -1 : 1;
    _bresenham.error = -1 * _stepsNeededFade;
    _bresenham.count = 0;

    _stepsNeededFadeAndStay = _stepsNeededFade + static_cast<int>(_stay / RGBWW_MINTIMEDIFF);

    return true;
}

bool AnimTransition::run() {
    if (_currentstep == 0) {
        if (!init()) {
            return true;
        }
        _currentstep = 0;
    }

    _currentstep++;
    if (_currentstep >= _stepsNeededFadeAndStay) {
        // ensure that the with the last step
        // we arrive at the destination color
        _value = _finalval;
        return true;
    }
    else if (_currentstep < _stepsNeededFade) {
        // we are fading. calculate new colors with bresenham
        _value = bresenham(_bresenham, _stepsNeededFade, _baseval, _value);
    }
    else {
        // we are in the stay phase, do nothing
    }

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
        values.error = values.error - 2 * dx;
        return base + ((values.count * values.step) >> 8);
    }
    return current;
}

///////////////////////////////

AnimTransitionCircularHue::AnimTransitionCircularHue(const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int stay, int direction, RGBWWLed const * rgbled,
        CtrlChannel ch, bool requeue, const String& name) :
        AnimTransition(endVal, ramp, stay, rgbled, ch, requeue, name), _direction(direction) {
}

AnimTransitionCircularHue::AnimTransitionCircularHue(const AbsOrRelValue& startVal, const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int stay, int direction,
        RGBWWLed const * rgbled, CtrlChannel ch, bool requeue, const String& name) :
        AnimTransition(startVal, endVal, ramp, stay, rgbled, ch, requeue, name) {
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
    d *= (_direction == 1) ? 1 : -1;

    switch (_ramp.type) {
    case RampTimeOrSpeed::Type::Time: {
        _stepsNeededFade = static_cast<int>(_ramp.value / RGBWW_MINTIMEDIFF);
        break;
    }
    case RampTimeOrSpeed::Type::Speed: {
        const uint32_t diff1 = abs(_finalval - _baseval);
        const uint32_t diff2 = RGBWW_CALC_HUEWHEELMAX - diff1;
        const uint32_t diff = (d == -1) ? max(diff1, diff2) : min(diff1, diff2);
        const double diffDegree = (static_cast<double>(diff) / RGBWW_CALC_HUEWHEELMAX) * 360;
        _stepsNeededFade = static_cast<int>((diffDegree / _ramp.value) * 60 * 1000 / RGBWW_MINTIMEDIFF);
        break;
    }
    }

    _stepsNeededFade = max(_stepsNeededFade, 1); //avoid 0 division

    //HUE
    _bresenham.delta = (d == -1) ? l : r;
    _bresenham.step = 1;
    _bresenham.step = (_bresenham.delta < _stepsNeededFade) ? (_bresenham.step << 8) : (_bresenham.delta << 8) / _stepsNeededFade;
    _bresenham.step *= d;
    _bresenham.error = -1 * _stepsNeededFade;
    _bresenham.count = 0;

    _stepsNeededFadeAndStay = _stepsNeededFade + static_cast<int>(_stay / RGBWW_MINTIMEDIFF);

    return true;
}

bool AnimTransitionCircularHue::run() {
    const bool result = AnimTransition::run();
    RGBWWColorUtils::circleHue(_value);
    return result;
}

AnimBlink::AnimBlink(int blinkTime, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue, const String& name) :
        RGBWWLedAnimation(rgbled, ch, Type::Blink, requeue, name) {
    if (blinkTime > 0) {
        _stepsNeeded = blinkTime / RGBWW_MINTIMEDIFF;
    }
}

bool AnimBlink::run() {
    if (_currentstep == 0)
        init();

    _currentstep += 1;
    if (_stepsNeeded != 0) {
        if (_currentstep < _stepsNeeded) {
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

    switch (_ctrlChannel) {
    case CtrlChannel::Hue:
        _value = _prevvalue + RGBWW_CALC_HUEWHEELMAX / 2;
        RGBWWColorUtils::circleHue(_value);
        break;
    default:
        _value = (_prevvalue > (RGBWW_CALC_MAXVAL / 2)) ? 0 : RGBWW_CALC_MAXVAL;
        break;
    }
    return true;
}

void AnimBlink::reset() {
    _currentstep = 0;
}
