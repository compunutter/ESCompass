#pragma once

#include <cmath>

// Screen geometry for the two compasses.
//
// Bearings are clockwise from geographic north. The caller supplies the
// screen angle of geographic north (0 = up the window, clockwise positive,
// y grows downward). A unit bearing vector then points away from the centre.

namespace ESCompass
{

struct Vec2
{
	double x;
	double y;
};

struct RayHit
{
	bool ok;
	double t;
	double x;
	double y;
};

inline double DegreesToRadians(double degrees)
{
	return degrees * 0.017453292519943295769;
}

inline double RadiansToDegrees(double radians)
{
	return radians * 57.295779513082320877;
}

// Clockwise from screen-up, in the same sense as a bearing.
inline double ScreenAngle(double dx, double dy)
{
	return std::atan2(dx, -dy);
}

inline Vec2 BearingVector(double northAngleRadians, double bearingDegrees)
{
	const double angle = northAngleRadians + DegreesToRadians(bearingDegrees);
	Vec2 vector;
	vector.x = std::sin(angle);
	vector.y = -std::cos(angle);
	return vector;
}

// First hit of a ray (centre + t * direction, t > 0) on the rectangle border.
// direction does not have to be unit length; t is in that vector's units.
inline RayHit IntersectBorder(
	double cx,
	double cy,
	double vx,
	double vy,
	double left,
	double top,
	double right,
	double bottom)
{
	RayHit best;
	best.ok = false;
	best.t = 1.0e300;
	best.x = 0.0;
	best.y = 0.0;

	const double edgeSlop = 1.0e-6;

	if (std::fabs(vx) > 1.0e-12)
	{
		const double sides[2] = { left, right };
		for (int i = 0; i < 2; ++i)
		{
			const double t = (sides[i] - cx) / vx;
			const double y = cy + t * vy;
			if (t > edgeSlop && t < best.t && y >= top - edgeSlop && y <= bottom + edgeSlop)
			{
				best.ok = true;
				best.t = t;
				best.x = sides[i];
				best.y = y;
			}
		}
	}

	if (std::fabs(vy) > 1.0e-12)
	{
		const double sides[2] = { top, bottom };
		for (int i = 0; i < 2; ++i)
		{
			const double t = (sides[i] - cy) / vy;
			const double x = cx + t * vx;
			if (t > edgeSlop && t < best.t && x >= left - edgeSlop && x <= right + edgeSlop)
			{
				best.ok = true;
				best.t = t;
				best.x = x;
				best.y = sides[i];
			}
		}
	}

	return best;
}

inline double NormalizeDegrees(double degrees)
{
	double wrapped = std::fmod(degrees, 360.0);
	if (wrapped < 0.0)
		wrapped += 360.0;
	return wrapped;
}

} // namespace ESCompass
