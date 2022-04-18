#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade_types.h"

namespace handmade {

	f64 Min(f64 a, f64 b);
	f64 Max(f64 a, f64 b);
	f64 Clamp(f64 x, f64 upper, f64 lower);
}

#endif // HANDMADE_MATH_H