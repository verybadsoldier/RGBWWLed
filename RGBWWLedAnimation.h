/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#pragma once
#include "RGBWWLedColor.h"
#include "RGBWWTypes.h"

class RGBWWLed;
class RGBWWLedAnimation;

/**
 * Abstract class representing the interface for animations
 *
 */
class RGBWWLedAnimation {
  public:
    enum class Type {
        Undefined,
        SetAndStay,
        Transition,
        Blink,
    };

    RGBWWLedAnimation(RGBWWLed const* rgbled, CtrlChannel ch, Type type, bool requeue = false, const String& name = "");

    virtual ~RGBWWLedAnimation(){};

    /**
     * Processing method, will be called from main loop
     *
     * @return status of the animation
     * @retval true     the animation is finished
     * @retval false    the animation is not finished yet
     */
    virtual bool run() {
        return true;
    };

    virtual const char* toString() {
        return "<empty>";
    }

    /**
     * Generic interface method for changing a variable
     * representing the speed of the current active animation
     *
     * @param newspeed
     */
    virtual void setSpeed(int newspeed){};

    /**
     * Generic interface method for changing a variable
     * representing the brightness of the current active animation
     *
     * @param newbrightness
     */
    virtual void setBrightness(int newbrightness){};

    /**
     * Interface to reset the animation so it can run from the beginning
     *
     */
    virtual void reset(){};

    bool shouldRequeue() const {
        return _requeue;
    }

    const String& getName() const {
        return _name;
    }

    int getAnimValue() const {
        return _value;
    }

    Type getAnimType() const {
        return _type;
    }

  protected:
    int getBaseValue() const;

    RGBWWLed const* _rgbled = nullptr;
    CtrlChannel _ctrlChannel = CtrlChannel::None;
    const bool _requeue = false;
    const String _name;
    int _value = 0;
    Type _type = Type::Undefined;
};

class AnimTransition : public RGBWWLedAnimation {
  public:
    AnimTransition(const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int stay, RGBWWLed const* rgbled,
                   CtrlChannel ch, bool requeue = false, const String& name = "");
    AnimTransition(const AbsOrRelValue& from, const AbsOrRelValue& endVal, const RampTimeOrSpeed& ramp, int stay,
                   RGBWWLed const* rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

  protected:
    int bresenham(BresenhamValues& values, int& dx, int& base, int& current);

    virtual bool init();

    int _baseval = 0;
    int _currentval = 0;
    int _finalval = 0;
    bool _hasfromval = false;
    int _currentstep = 0;
    int _stepsNeededFade = 0; // steps for fading
    int _stepsNeededFadeAndStay =
        0; // steps for fading + staying (after the fade). so this is the total number of steps

    BresenhamValues _bresenham;
    AbsOrRelValue _initEndVal;
    AbsOrRelValue _initStartVal;
    RampTimeOrSpeed _ramp;
    int _stay = 0; // milliseconds
};

class AnimBlink : public RGBWWLedAnimation {
  public:
    AnimBlink(int blinkTime, RGBWWLed const* rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

  private:
    virtual bool init();

    int _currentstep = 0;
    int _stepsNeeded = 0;

  protected:
    int _prevvalue = 0;
};
