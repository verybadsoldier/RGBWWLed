/**
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 * @version 0.7.3
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 * This Library provides Methods for controlling LED RGB, WarmWhite and
 * ColdWhite Led(stripes) via PWM. The Library can either use the standard
 * ARDUINO PWM Implementation or if available the espressif ESP8266 SDK
 * Pwm.
 *
 * The Library can handle different color modes (RGB, RGB+WarmWhite,
 * RGB+ColdWhite, RGB+WarmWhite+ColdWhite) and offeres different calculation
 * models for outputting HSV colors
 *
 * The Library offers various adjustments (i.e. maximum brightness of each
 * channel, adjusting HSV basecolors etc..)
 *
 * It also provides a simple interface for creating Animations and
 * queuing several animations/colors to a complex animation
 */

#ifndef RGBWWLed_h
#define RGBWWLed_h
#include <Arduino.h>
#include "../../Wiring/WHashMap.h"
#ifdef SMING_VERSION
	#define RGBWW_USE_ESP_HWPWM
	#include "../../SmingCore/SmingCore.h"
	#define RGBWW_PWMRESOLUTION 65536
	#define RGBWW_CALC_DEPTH 10
#else
	#define RGBWW_PWMRESOLUTION 1023
	#define RGBWW_CALC_DEPTH 8
#endif

#define RGBWW_VERSION "0.8.1-vbs1b"
#define RGBWW_CALC_WIDTH int(pow(2, RGBWW_CALC_DEPTH))
#define	RGBWW_CALC_MAXVAL int(RGBWW_CALC_WIDTH - 1)
#define	RGBWW_CALC_HUEWHEELMAX int(RGBWW_CALC_MAXVAL * 6)


#define RGBWW_UPDATEFREQUENCY 50
#define RGBWW_MINTIMEDIFF  int(1000 / RGBWW_UPDATEFREQUENCY)
#define RGBWW_ANIMATIONQSIZE 100
#define	RGBWW_WARMWHITEKELVIN 2700
#define RGBWW_COLDWHITEKELVIN 6000


#ifndef DEBUG_RGBWW
	#define DEBUG_RGBWW 0
#endif

#include "debugUtils.h"
#include "RGBWWconst.h"
#include "RGBWWLedColor.h"
#include "RGBWWLedAnimation.h"
#include "RGBWWLedOutput.h"



class RGBWWLedAnimation;
class RGBWWLedAnimationQ;
class RGBWWColorUtils;
class PWMOutput;
class RGBWWAnimatedChannel;

enum class QueuePolicy {
    FrontReset, // queue to front and let the original anim run from the beginning afterwards
    Front, // queue to front and the current animation will continue where it was interrupted
    Back,
    Single,
};

/**
 *
 */
class RGBWWLed
{
public:
	RGBWWLed();
	~RGBWWLed();

	/**
	 * Initialize the the LED Controller
	 *
	 * @param redPIN	int representing the MC pin for the red channel
	 * @param greenPIN  int representing the MC pin for the green channel
	 * @param bluePIN   int representing the MC pin for the blue channel
	 * @param wwPIN     int representing the MC pin for the warm white channel
	 * @param cwPIN		int representing the MC pin for the cold white channel
	 * @param pwmFrequency (default 200)
	 */
	void init(int redPIN, int greenPIN, int bluePIN, int wwPIN, int cwPIN, int pwmFrequency=200);


	/**
	 * Main function for processing animations/color output
	 * Use this in your loop()
	 *
	 *
	 * @retval true 	not updating
	 * @retval false 	updates applied
	 */
	bool show();


	/**
	 * Refreshs the current output.
	 * Usefull when changing brightness, white or color correction
	 *
	 */
	void refresh();


	/**
	 * Set Output to given HSVK color.
	 * Converts HSVK into seperate color channels (r,g,b,w)
	 * and applies brightness and white correction
	 *
	 * @param HSVK&	outputcolor
	 */
	void setOutput(HSVCT& color);


	/**
	 * Sets the output of the Controller to the given RGBWK
	 * while applying brightness and white correction
	 *
	 * @param RGBWK& outputcolor
	 */
	void setOutput(RGBWCT& color);

	/**
	 * Sets the output of the controller to the specified
	 * channel values (with internal brightness correction)
	 */
	void setOutput(ChannelOutput& output);


	/**
	 * Directly set the PWM values without color correction or white balance.
	 * Assumes the values are in the range of [0, PWMMAXVAL].
	 *
	 * @param int&	red
	 * @param int&	green
	 * @param int&	blue
	 * @param int&	wwhite
	 * @param int&	cwhite
	 */
	void setOutputRaw(int& red, int& green, int& blue, int& cwhite, int& wwhite);


	/**
	 * Returns an HSVK object representing the current color
	 *
	 * @return HSVK current color
	 */
	HSVCT getCurrentColor();

	/**
	 * Returns the current values for each channel
	 *
	 * #return ChannelOutput current value of all channels
	 */
	ChannelOutput getCurrentOutput();


	/**
	 * 	Output specified color
	 *  (until a new color is set)
	 *
	 * @param color
	 * @param queue
	 */
	void setHSV(HSVCT& color, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");


	/**
	 * Output color for time x
	 * if time x has passed it will continue with the next
	 * color/transition in the animat queue
	 *
	 * @param color
	 * @param time
	 * @param queue
	 */
	void setHSV(HSVCT& color, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");


	/**
	 * Fade to specified HSVK color
	 *
	 * @param color 	new color
	 * @param time		duration of transition in ms
	 * @param direction direction of transition (0= long/ 1=short)
	 */
	void fadeHSV(HSVCT& color, int time, int direction, bool requeue = false, const String& name = "");


	/**
	 * Fade to specified HSVK color
	 *
	 * @param color 	new color
	 * @param time		duration of transition in ms
	 * @param queue		directly execute fade or queue it
	 */
	void fadeHSV(HSVCT& color, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");


	/**
	 * Fade to specified HSVK color
	 *
	 * @param color 	new color
	 * @param time		duration of transition in ms
	 * @param direction direction of transition (0= long/ 1=short)
	 * @param queue		directly execute fade or queue it
	 */
	void fadeHSV(HSVCT& color, int time, int direction = 1, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");

#if 0
	/**
	 * Fade from one color (colorFrom) to another color
	 *
	 * @param colorFrom starting color of transition
	 * @param color 	ending color of transition
	 * @param time		duration of transition in ms
	 * @param direction direction of transition (0= long/ 1=short)
	 * @param queue		directly execute fade or queue it
	 */
	void fadeHSV(HSVCT& colorFrom, HSVCT& color, int time, int direction = 1, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");
#endif
	//TODO: add documentation
	/**
	 *
	 * @param output
	 */
	void setRAW(ChannelOutput output, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");

	//TODO: add documentation
	/**
	 *
	 * @param output
	 * @param time
	 * @param queue
	 */
	void setRAW(ChannelOutput output, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "");



	//TODO: add documentation
	/**
	 *
	 * @param output
	 * @param time
	 * @param queue
	 */
	void fadeRAW(ChannelOutput output, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "" );

#if 0
	//TODO: add documentation
	/**
	 *
	 * @param output_from
	 * @param output
	 * @param time
	 * @param queue
	 */
	void fadeRAW(ChannelOutput output_from, ChannelOutput output, int time, QueuePolicy queuePolicy = QueuePolicy::Single, bool requeue = false, const String& name = "" );
#endif


	void blink();

	//colorutils
	RGBWWColorUtils colorutils;

	void pauseAnimation();
	void continueAnimation();

private:
	enum class CtrlChannel {
	    Hue,
	    Sat,
	    Val,
	    ColorTemp,

		Red,
		Green,
		Blue,
		ColdWhite,
		WarmWhite,
	};

    enum class ColorMode {
        Hsv,
        Rgb,
        Raw,
    };

	ChannelOutput  	_current_output;
	HSVCT 			_current_color;

	PWMOutput* 		_pwm_output;

	HashMap<CtrlChannel, RGBWWAnimatedChannel*> _animChannelsHsv;
	HashMap<CtrlChannel, RGBWWAnimatedChannel*> _animChannelsRaw;

	void (*_animationcallback)(RGBWWLed* led, RGBWWLedAnimation* anim) = nullptr;

	ColorMode _mode = ColorMode::Hsv;
};

#endif //RGBWWLed_h
