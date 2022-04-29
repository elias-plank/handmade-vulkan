#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade_types.h"

#include <cmath>

namespace handmade {
	
	f64 Min(f64 a, f64 b);
	f64 Max(f64 a, f64 b);
	f64 Clamp(f64 x, f64 upper, f64 lower);
	
	Vector3 Add(Vector3* left, Vector3* right);
	Vector3 Sub(Vector3* left, Vector3* right);
	Vector3 Mult(Vector3* left, Vector3* right);
	Vector3 Div(Vector3* left, Vector3* right);
	
	f32 Dot(Vector3* left, Vector3* right);
	f32 Length(Vector3* vector);
	
	Vector3 Unit(Vector3* vector);
}
// NOTE(Elias): 
#endif // HANDMADE_MATH_H