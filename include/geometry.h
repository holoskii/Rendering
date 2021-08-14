// containing geometry objects: 2d vector, 3d vector, 4x4 transformation matrix
#pragma once

#include <cstdlib> 
#include <cstdio> 
#include <iostream> 
#include <iomanip> 
#include <cmath> 

template<typename T>
class Vec2
{
public:
	Vec2() : x(0), y(0) {}

	Vec2(T a_x) : x(a_x), y(a_x) {}

	Vec2(T a_x, T a_y) : x(a_x), y(a_y) {}

	Vec2 operator + (const Vec2& v) const
	{
		return Vec2(x + v.x, y + v.y);
	}

	Vec2 operator / (const T& r) const
	{
		return Vec2(x / r, y / r);
	}

	Vec2 operator * (const T& r) const
	{
		return Vec2(x * r, y * r);
	}

	Vec2& operator /= (const T& r)
	{
		x /= r, y /= r; return *this;
	}

	Vec2& operator *= (const T& r)
	{
		x *= r, y *= r; return *this;
	}

	friend std::ostream& operator << (std::ostream& s, const Vec2<T>& v)
	{
		return s << '[' << v.x << ' ' << v.y << ']';
	}

	friend Vec2 operator * (const T& r, const Vec2<T>& v)
	{
		return Vec2(v.x * r, v.y * r);
	}

	T x, y;
};
typedef Vec2<float> Vec2f;
typedef Vec2<int> Vec2i;

template<typename T>
class Vec3
{
public:
	Vec3() : x(0), y(0), z(0) {}

	Vec3(T a_x) : x(a_x), y(a_x), z(a_x) {}

	Vec3(T a_x, T a_y, T a_z) : x(a_x), y(a_y), z(a_z) {}

	T dotProduct(const Vec3<T>& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Vec3 crossProduct(const Vec3<T>& v) const
	{
		return Vec3<T>(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	T length2() const
	{
		return x * x + y * y + z * z;
	}

	T length() const
	{
		return sqrt(length2());
	}

	Vec3& normalize()
	{
		T len2 = length2();
		if (len2 > 0) {
			T factor = 1 / sqrt(len2);
			x *= factor, y *= factor, z *= factor;
		}
		return *this;
	}

	Vec3 operator - () const
	{
		return Vec3(-x, -y, -z);
	}

	Vec3 operator + (const Vec3& v) const
	{
		return Vec3(x + v.x, y + v.y, z + v.z);
	}

	Vec3 operator - (const Vec3& v) const
	{
		return Vec3(x - v.x, y - v.y, z - v.z);
	}

	Vec3 operator * (const Vec3& v) const
	{
		return Vec3(x * v.x, y * v.y, z * v.z);
	}

	Vec3 operator / (const Vec3& v) const
	{
		return Vec3(x / v.x, y / v.y, z / v.z);
	}

	Vec3 operator * (const T& r) const
	{
		return Vec3(x * r, y * r, z * r);
	}

	Vec3 operator / (const T& r) const
	{
		return Vec3(x / r, y / r, z / r);
	}

	Vec3& operator += (const Vec3<T>& v)
	{
		x += v.x, y += v.y, z += v.z; return *this;
	}

	Vec3& operator /= (const T& r)
	{
		x /= r, y /= r, z /= r; return *this;
	}

	Vec3& operator *= (const T& r)
	{
		x *= r, y *= r, z *= r; return *this;
	}

	friend Vec3 operator * (const T& r, const Vec3& v)
	{
		return Vec3<T>(v.x * r, v.y * r, v.z * r);
	}

	friend Vec3 operator / (const T& r, const Vec3& v)
	{
		return Vec3<T>(r / v.x, r / v.y, r / v.z);
	}

	friend std::ostream& operator << (std::ostream& s, const Vec3<T>& v)
	{
		return s << '[' << v.x << ' ' << v.y << ' ' << v.z << ']';
	}

	const T& operator [] (uint8_t i) const
	{
		return (&x)[i];
	}

	T& operator [] (uint8_t i)
	{
		return (&x)[i];
	}

	T x, y, z;
};
typedef Vec3<float> Vec3f;
typedef Vec3<int> Vec3i;

template<typename T>
class Matrix44
{
public:

	T x[4][4] = {
		{1,0,0,0},
		{0,1,0,0},
		{0,0,1,0},
		{0,0,0,1}
	};

	Matrix44() {}

	Matrix44(T a, T b, T c, T d, T e, T f, T g, T h,
		T i, T j, T k, T l, T m, T n, T o, T p)
	{
		x[0][0] = a;
		x[0][1] = b;
		x[0][2] = c;
		x[0][3] = d;
		x[1][0] = e;
		x[1][1] = f;
		x[1][2] = g;
		x[1][3] = h;
		x[2][0] = i;
		x[2][1] = j;
		x[2][2] = k;
		x[2][3] = l;
		x[3][0] = m;
		x[3][1] = n;
		x[3][2] = o;
		x[3][3] = p;
	}

	const T* operator [] (uint8_t i) const
	{
		return x[i];
	}

	T* operator [] (uint8_t i)
	{
		return x[i];
	}

	Matrix44 operator * (const Matrix44& v) const
	{
		Matrix44 tmp;
		return multiply(*this, v);
	}

	static Matrix44 multiply(const Matrix44<T>& a, const Matrix44& b)
	{
		Matrix44 c;
		for (uint8_t i = 0; i < 4; ++i)
			for (uint8_t j = 0; j < 4; ++j)
				c[i][j] =
				a[i][0] * b[0][j] + a[i][1] * b[1][j] +
				a[i][2] * b[2][j] + a[i][3] * b[3][j];
		return c;
	}

	Matrix44 transposed() const
	{
		Matrix44 t;
		for (uint8_t i = 0; i < 4; ++i)
			for (uint8_t j = 0; j < 4; ++j)
				t[i][j] = x[j][i];
		return t;
	}

	Matrix44& transpose()
	{
		Matrix44 tmp(
			x[0][0],
			x[1][0],
			x[2][0],
			x[3][0],
			x[0][1],
			x[1][1],
			x[2][1],
			x[3][1],
			x[0][2],
			x[1][2],
			x[2][2],
			x[3][2],
			x[0][3],
			x[1][3],
			x[2][3],
			x[3][3]
		);
		*this = tmp;

		return *this;
	}

	template<typename S>
	Vec3<S> multVecMatrix(const Vec3<S>& src) const
	{
		Vec3f dst;
		S a, b, c, w;

		dst.x = src[0] * x[0][0] + src[1] * x[1][0] + src[2] * x[2][0] + x[3][0];
		dst.y = src[0] * x[0][1] + src[1] * x[1][1] + src[2] * x[2][1] + x[3][1];
		dst.z = src[0] * x[0][2] + src[1] * x[1][2] + src[2] * x[2][2] + x[3][2];
		w = src[0] * x[0][3] + src[1] * x[1][3] + src[2] * x[2][3] + x[3][3];

		if (w != 0.0f && w != -0.0f && w != 1.0f) {
			const float wInv = 1.0f / w;
			dst.x *= wInv;
			dst.y *= wInv;
			dst.z *= wInv;
		}
		return dst;
	}

	Matrix44 inverse() const
	{
		int i, j, k;
		Matrix44 s;
		Matrix44 t(*this);

		for (i = 0; i < 3; i++) {
			int pivot = i;

			T pivotsize = t[i][i];

			if (pivotsize < 0)
				pivotsize = -pivotsize;

			for (j = i + 1; j < 4; j++) {
				T tmp = t[j][i];

				if (tmp < 0)
					tmp = -tmp;

				if (tmp > pivotsize) {
					pivot = j;
					pivotsize = tmp;
				}
			}

			if (pivotsize == 0) {
				return Matrix44();
			}

			if (pivot != i) {
				for (j = 0; j < 4; j++) {
					T tmp;

					tmp = t[i][j];
					t[i][j] = t[pivot][j];
					t[pivot][j] = tmp;

					tmp = s[i][j];
					s[i][j] = s[pivot][j];
					s[pivot][j] = tmp;
				}
			}

			for (j = i + 1; j < 4; j++) {
				T f = t[j][i] / t[i][i];

				for (k = 0; k < 4; k++) {
					t[j][k] -= f * t[i][k];
					s[j][k] -= f * s[i][k];
				}
			}
		}

		for (i = 3; i >= 0; --i) {
			T f;

			if ((f = t[i][i]) == 0) {
				return Matrix44();
			}

			for (j = 0; j < 4; j++) {
				t[i][j] /= f;
				s[i][j] /= f;
			}

			for (j = 0; j < i; j++) {
				f = t[j][i];

				for (k = 0; k < 4; k++) {
					t[j][k] -= f * t[i][k];
					s[j][k] -= f * s[i][k];
				}
			}
		}

		return s;
	}

	const Matrix44<T>& invert()
	{
		*this = inverse();
		return *this;
	}

	friend std::ostream& operator << (std::ostream& s, const Matrix44& m)
	{
		std::ios_base::fmtflags oldFlags = s.flags();
		int width = 12;
		s.precision(5);
		s.setf(std::ios_base::fixed);

		s << "[" << std::setw(width) << m[0][0] <<
			" " << std::setw(width) << m[0][1] <<
			" " << std::setw(width) << m[0][2] <<
			" " << std::setw(width) << m[0][3] << "\n" <<

			" " << std::setw(width) << m[1][0] <<
			" " << std::setw(width) << m[1][1] <<
			" " << std::setw(width) << m[1][2] <<
			" " << std::setw(width) << m[1][3] << "\n" <<

			" " << std::setw(width) << m[2][0] <<
			" " << std::setw(width) << m[2][1] <<
			" " << std::setw(width) << m[2][2] <<
			" " << std::setw(width) << m[2][3] << "\n" <<

			" " << std::setw(width) << m[3][0] <<
			" " << std::setw(width) << m[3][1] <<
			" " << std::setw(width) << m[3][2] <<
			" " << std::setw(width) << m[3][3] << "]";

		s.flags(oldFlags);
		return s;
	}
};
typedef Matrix44<float> Matrix44f;

enum class RayType { PrimaryRay, ShadowRay };
class Ray
{
public:
	RayType rayType;
	Vec3f orig;
	Vec3f dir;

	Ray(const Vec3f& a_orig = { 0,0,0 }, const Vec3f& a_dir = { 0,0,-1 }, const RayType a_rayType = RayType::PrimaryRay)
		: orig(a_orig), dir(a_dir), rayType(a_rayType) {}
};