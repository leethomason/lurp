#pragma once

#include "assert.h"
#include <functional>

namespace tween {

inline double linear(double t) { return t; }
inline double easeIn(double t) { return t * t; }
inline double easeOut(double t) { return 1 - (1 - t) * (1 - t); }
inline double cosine(double t) { return 0.5 - 0.5 * cos(t * 3.14159265358979323846); }

class Tween
{
public:
	Tween() = default;
	~Tween() = default;

	Tween(double start) {
		_value = start;
		_index = 0;
	}

	void set(double value) {
		_steps.push_back({0, value, value, linear});
	}

    // Add a step to the tween
    // @param duration: time in seconds for this step
    // @param target: value to tween to (always tween from the value of the last step)
    // @param func: easing function to use for this step. The func takes in a double from 
    //              0 to 1 and returns a double from 0 to 1
	void add(double duration, double target, std::function<double(double)> func = linear) {
		if (_steps.empty())
			_steps.push_back({duration, _value, target, func});
		else
			_steps.push_back({duration, _steps.back().end, target, func});
	}

	void addASR(double attack, double hold, double release, double start, double end, 
		std::function<double(double)> func = linear) 
	{
		add(attack, end, func);
		add(hold, end, linear);
		add(release, start, func);
	}

	void addASR(double attack, double hold, double release, double start, double end, 
		std::function<double(double)> func0, 
		std::function<double(double)> func1) 
	{
		add(attack, end, func0);
		add(hold, end, linear);
		add(release, start, func1);
	}

	double value() const {
		return _value;
	}

	void tick(double dt) {
		_dt += dt;
		while(!done() && _dt >= _steps[_index].duration) {
			_value = _steps[_index].end;
			_dt -= _steps[_index].duration;
			_index++;
		}
		if (!done()) {
			assert(_dt < _steps[_index].duration);
			const Step& step = _steps[_index];
			double uniform = _dt / _steps[_index].duration;

			_value = step.start + (step.func(uniform) * (step.end - step.start));
		}
	}

	bool done() const {
		return _index == _steps.size();
	}

private:
	struct Step {
		double duration = 0;
		double start = 0;
		double end = 0;
		std::function<double(double)> func = linear;
	};

	size_t _index = 0;          // current step
	double _dt = 0.0;           // time into current step
	double _value = 0.0;        // current value
	std::vector<Step> _steps;
};

} // namespace tween