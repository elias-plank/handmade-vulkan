#include "handmade_math.h"

namespace handmade {

	f64 Min(f64 a, f64 b) {

		return a < b ? a : b;
	}

	f64 Max(f64 a, f64 b) {

		return a > b ? a : b;
	}

	f64 Clamp(f64 x, f64 upper, f64 lower) {

		return Min(upper, Max(x, lower));
	}
}