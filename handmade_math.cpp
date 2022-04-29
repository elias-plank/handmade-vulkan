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
	
	Vector3 Add(Vector3* left, Vector3* right) {
		
		Vector3 result{};
		result.X = left->X + right->X;
		result.Y = left->Y + right->Y;
		result.Z = left->Z + right->Z;
		
		return result;
	}
	
	Vector3 Sub(Vector3* left, Vector3* right) {
		
		Vector3 result{};
		result.X = left->X - right->X;
		result.Y = left->Y - right->Y;
		result.Z = left->Z - right->Z;
		
		return result;
	}
	
	Vector3 Mult(Vector3* left, Vector3* right) {
		
		Vector3 result{};
		result.X = left->X * right->X;
		result.Y = left->Y * right->Y;
		result.Z = left->Z * right->Z;
		
		return result;
	}
	
	Vector3 Div(Vector3* left, Vector3* right) {
		
		Vector3 result{};
		result.X = left->X / right->X;
		result.Y = left->Y / right->Y;
		result.Z = left->Z / right->Z;
		
		return result;
	}
	
	f32 Dot(Vector3* left, Vector3* right) {
		
		return left->X * right->X + left->Y * right->Y + left->Z * right->Z;
	}
	
	f32 Length(Vector3* vector) {
		
		return sqrt(vector->X * vector->X + vector->Y * vector->Y + vector->Z * vector->Z);
	}
	
	Vector3 Unit(Vector3* vector) {
		
		f32 length = Length(vector);
		
		Vector3 result{};
		result.X = vector->X / length;
		result.Y = vector->Y / length;
		result.Z = vector->Z / length;
		
		return result;
	}
}