/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */

#pragma once
#include "RGBWWTypes.h"
#include "RGBWWLedColor.h"


class RGBWWLed;
class RGBWWLedAnimation;


/**
 * Abstract class representing the interface for animations
 *
 */
class RGBWWLedAnimation
{
public:
    RGBWWLedAnimation(RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual ~RGBWWLedAnimation() {};
    /**
     * Processing method, will be called from main loop
     *
     * @return status of the animation
     * @retval true     the animation is finished
     * @retval false    the animation is not finished yet
     */
    virtual bool run() {return true;};

    virtual const char* toString() {
        return "<empty>";
    }

    /**
     * Generic interface method for changing a variable
     * representing the speed of the current active animation
     *
     * @param newspeed
     */
    virtual void setSpeed(int newspeed) {};

    /**
     * Generic interface method for changing a variable
     * representing the brightness of the current active animation
     *
     * @param newbrightness
     */
    virtual void setBrightness(int newbrightness) {};

    /**
     * Interface to reset the animation so it can run from the beginning
     *
     */
    virtual void reset() {};

    bool shouldRequeue() const { return _requeue; }

    const String& getName() const { return _name; }

    int getAnimValue() const { return _value; }


protected:
    int getBaseValue() const;

    RGBWWLed const * _rgbled = nullptr;
    CtrlChannel _ctrlChannel = CtrlChannel::None;
    const bool _requeue = false;
    const String _name;
    int _value = 0;
};

class AnimSetAndStay : public RGBWWLedAnimation {
public:
    AnimSetAndStay(const AbsOrRelValue& endVal, int onTime, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

private:
    virtual bool init();

    int _currentstep = 0;
    int _steps = 0;
    AbsOrRelValue _initEndVal;
};

class AnimTransition : public RGBWWLedAnimation {
public:
    AnimTransition(const AbsOrRelValue& endVal, int ramp, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");
    AnimTransition(const AbsOrRelValue& from, const AbsOrRelValue&  endVal, int ramp, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;
    virtual void reset() override;

protected:
    int bresenham(BresenhamValues& values, int& dx, int& base, int& current);

    virtual bool init();

    int 				_baseval = 0;
    int   				_currentval = 0;
    int   				_finalval = 0;
    bool  				_hasfromval = false;
    int  				_currentstep = 0;
    int  				_steps = 0;
    BresenhamValues 	_bresenham;
    AbsOrRelValue 		_initEndVal;
    AbsOrRelValue       _initStartVal;
};

class AnimTransitionCircularHue : public AnimTransition {
public:
	AnimTransitionCircularHue(const AbsOrRelValue& endVal, int ramp, int direction, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");
	AnimTransitionCircularHue(const AbsOrRelValue& startVal, const AbsOrRelValue& endVal, int ramp, int direction, RGBWWLed const * rgbled, CtrlChannel ch, bool requeue = false, const String& name = "");

    virtual bool run() override;

private:
    virtual bool init() override;

    int _direction = 0;
};

#if 0
/**
 * Set output to color without effect/transition
 *
 */
class HSVSetOutput: public RGBWWLedAnimation
{
public:

    /**
     * Set output to color without effect/transition
     *
     * @param color color to show
     * @param ctrl  pointer to RGBWWLed controller object
     * @param time  minimal amount of time the color stays active
     */
    HSVSetOutput(const HSVCT& color, RGBWWLed* rgbled, int time = 0, bool requeue = false, const String& name = "");

    bool run();


private:
    int count;
    int steps;
    RGBWWLed* rgbwwctrl;
    HSVCT outputcolor;
};

/**
 * A simple Colorfade utilizing the HSV colorspace
 *
 */
class HSVTransition: public RGBWWLedAnimation
{
public:


    /**
     * Fade from the current color to another color (colorEnd).
     *
     * @param colorEnd      color at the end
     * @param time          the amount of time the transition takes in ms
     * @param direction     shortest (direction == 0)/longest (direction == 1) way for transition
     * @param ctrl          main RGBWWLed object for calling setOutput
     */
    HSVTransition(const HSVCT& colorEnd, const int& time, const int& direction, RGBWWLed* ctrl, bool requeue = false, const String& name = "");

    /**
     * Fade from one color (colorFrom) to another color (colorFinish)
     *
     * @param colorFrom     color at the beginning
     * @param colorEnd      color at the end of the transition
     * @param time          the amount of time the transition takes in ms
     * @param direction     shortest (direction == 0)/longest (direction == 1) way for transition
     * @param ctrl          main RGBWWLed object for calling setOutput
     */
    HSVTransition(const HSVCT& colorFrom, const HSVCT& colorEnd, const int& time, const int& direction, RGBWWLed* ctr, bool requeue = false, const String& name = "");

    void reset();
    bool run();

private:
    bool init();

    HSVCT   _basecolor;
    HSVCT   _currentcolor;
    HSVCT   _finalcolor;
    bool    _hasbasecolor;
    int _currentstep;
    int _steps;
    int _huedirection;
    BresenhamValues hue;
    BresenhamValues sat;
    BresenhamValues val;
    BresenhamValues ct;


    RGBWWLed*    rgbwwctrl;
    static  int bresenham(BresenhamValues& values, int& dx, int& base, int& current);
};



/**
 * Set output to a new state without effect/transition
 *
 */
class RAWSetOutput: public RGBWWLedAnimation
{
public:

    /**
     * Set output to color without effect/transition
     *
     * @param output output to set
     * @param ctrl  pointer to RGBWWLed controller object
     * @param time  minimal amount of time the output stays active
     */
    RAWSetOutput(const ChannelOutput& output, RGBWWLed* rgbled, int time = 0, bool requeue = false, const String& name = "");

    bool run();


private:
    int count;
    int steps;
    RGBWWLed* rgbwwctrl;
    ChannelOutput outputcolor;
};


/**
 * A simple Fade from one output state to another
 *
 */
class RAWTransition: public RGBWWLedAnimation
{
public:


    /**
     * Fade from the current output to a new one (output)
     *
     * @param output        output at the end of the transition
     * @param time          the amount of time the transition takes in ms
     * @param ctrl          main RGBWWLed object for calling setOutput
     */
    RAWTransition(const ChannelOutput& output, const int& time, RGBWWLed* ctrl, bool requeue = false, const String& name = "");

    /**
     * Fade from one output state (output_from) to another(output)
     *
     * @param output_from   output at the beginning
     * @param output        output at the end of the transition
     * @param time          the amount of time the transition takes in ms
     * @param ctrl          main RGBWWLed object for calling setOutput
     */
    RAWTransition(const ChannelOutput& output_from, const ChannelOutput& output, const int& time, RGBWWLed* ctrl, bool requeue = false, const String& name = "");

    void reset();
    bool run();

private:
    bool init();

    ChannelOutput   _basecolor;
    ChannelOutput   _currentcolor;
    ChannelOutput   _finalcolor;
    bool    _hasbasecolor;
    int _currentstep;
    int _steps;
    BresenhamValues red;
    BresenhamValues green;
    BresenhamValues blue;
    BresenhamValues warmwhite;
    BresenhamValues coldwhite;


    RGBWWLed*    rgbwwctrl;
    static  int bresenham(BresenhamValues& values, int& dx, int& base, int& current);
};


/**
 *
 */
class RGBWWAnimationSet: public RGBWWLedAnimation {
public:
    /**
     *
     * @param animations
     * @param count
     * @param loop
     */
    RGBWWAnimationSet(RGBWWLedAnimation** animations, int count, bool loop=false, bool requeue = false, const String& name = "");

    ~RGBWWAnimationSet();


    /**
     *
     * @param newspeed
     */
    void setSpeed(int newspeed);


    /**
     *
     * @param newbrightness
     */
    void setBrightness(int newbrightness);

    bool run();

private:
    int _current = 0;
    int _currentstep;
    int _brightness = -1;
    int _speed = -1;
    bool _loop;
    RGBWWLedAnimation** q;

};

#endif
