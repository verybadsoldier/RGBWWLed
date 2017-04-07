#pragma once

#include "../../SmingCore/SmingCore.h"


class AbsOrRelValue {
public:
	enum class Mode {
		Absolute,
		Relative,
	};

	AbsOrRelValue(const String& value) {
		if (value.startsWith("+") || value.startsWith("-")) {
			_mode = Mode::Relative;
			_value = value.substring(1).toFloat();
		}
		else {
			_mode = Mode::Absolute;
			_value = value.toFloat();
		}
	}

	AbsOrRelValue(AbsOrRelValue::Mode mode, int value) : _mode(mode), _value(value) {
	}

	AbsOrRelValue(float value) : _mode(Mode::Absolute), _value(value) {
	}

    bool operator==(const AbsOrRelValue& obj) const {
        return (_mode == obj.getMode()) && (this->_value == obj.getValue());
    }

	Mode getMode() const {
		return _mode;
	}

	int getValue() const {
		return _value;
	}

	int getFinalValue(int val) {
		switch(_mode) {
		case Mode::Relative:
			return _value + val;
		case Mode::Absolute:
			return _value;
		}
	}

private:
	AbsOrRelValue::Mode _mode;
	int _value;
};

template< typename T >
class Optional {
public:
	Optional() : _hasValue(false) {
	}

	Optional<T>(T value) : _value(value), _hasValue(true) {
	}

	T& operator=(const T& obj) {
		_value = obj;
		_hasValue = true;
	}

	const T& getValue() const {
		return _value;
	}

    bool operator==(const Optional<T>& obj) const {
        return (this->hasValue() == obj.hasValue()) && (this->getValue() == obj.getValue());
    }

    operator const T&() {
    	return getValue();
    }

	void clear() {
		_hasValue;
	}

	bool hasValue() const {
		return _hasValue;
	}
private:
	T _value;
	bool _hasValue = false;
};

struct RequestHSVCT {
	Optional<AbsOrRelValue> h;
	Optional<AbsOrRelValue> s;
	Optional<AbsOrRelValue> v;
	Optional<AbsOrRelValue> ct;

    bool operator==(const RequestHSVCT& obj) const {
        return this->h == obj.h &&
                this->s == obj.s &&
                this->v == obj.v &&
                this->ct == obj.ct;
    }

    bool operator!=(const RequestHSVCT& obj) const {
        return !(*this == obj);
    }

    RequestHSVCT& operator= (const RequestHSVCT& hsvct)
    {
    	this->h = hsvct.h;
    	this->s = hsvct.s;
    	this->v = hsvct.v;
    	this->ct = hsvct.ct;
        return *this;
    }
};

struct RequestChannel {
	Optional<AbsOrRelValue> r;
	Optional<AbsOrRelValue> g;
	Optional<AbsOrRelValue> b;
	Optional<AbsOrRelValue> ww;
	Optional<AbsOrRelValue> cw;

    bool operator==(const RequestChannel& ch) const {
        return this->r == ch.r &&
                this->g == ch.g &&
                this->b == ch.b &&
                this->ww == ch.ww &&
                this->cw == ch.cw;
    }

    bool operator!=(const RequestChannel& obj) const {
        return !(*this == obj);
    }

    RequestChannel& operator= (const RequestChannel& ch) {
    	this->r = ch.r;
    	this->g = ch.g;
    	this->b = ch.b;
    	this->cw = ch.cw;
    	this->ww = ch.ww;
        return *this;
    }
};

enum class ColorMode {
	Hsv,
	Raw,
};

enum class CtrlChannel {
	None,

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

struct BresenhamValues {
    int delta, error, count, step;
};

enum class QueuePolicy {
    FrontReset, // queue to front and let the original anim run from the beginning afterwards
    Front, // queue to front and the current animation will continue where it was interrupted
    Back,
    Single,
};

