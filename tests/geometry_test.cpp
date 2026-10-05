#include "../ESCompass/Geometry.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{

int g_Failures = 0;

void Expect(bool ok, const char* name)
{
	if (!ok)
	{
		std::cerr << "FAIL " << name << "\n";
		++g_Failures;
	}
}

void ExpectNear(const char* name, double got, double want, double tol)
{
	if (std::fabs(got - want) > tol)
	{
		std::cerr << "FAIL " << name << " got " << got << " want " << want << "\n";
		++g_Failures;
	}
}

void ExpectBearingHits(double north, double bearing, double cx, double cy, double left, double top, double right, double bottom)
{
	const ESCompass::Vec2 vector = ESCompass::BearingVector(north, bearing);
	const ESCompass::RayHit hit = ESCompass::IntersectBorder(cx, cy, vector.x, vector.y, left, top, right, bottom);
	Expect(hit.ok, "ray hits the border");
	if (!hit.ok)
		return;

	const bool onEdge =
		std::fabs(hit.x - left) < 1.0e-4 ||
		std::fabs(hit.x - right) < 1.0e-4 ||
		std::fabs(hit.y - top) < 1.0e-4 ||
		std::fabs(hit.y - bottom) < 1.0e-4;
	Expect(onEdge, "hit is on an edge");

	const double recovered = ESCompass::NormalizeDegrees(
		ESCompass::RadiansToDegrees(ESCompass::ScreenAngle(hit.x - cx, hit.y - cy) - north));
	const double delta = std::fabs(ESCompass::NormalizeDegrees(recovered - bearing));
	const double wrapped = delta > 180.0 ? 360.0 - delta : delta;
	ExpectNear("bearing recovered from hit", wrapped, 0.0, 1.0e-4);
}

} // namespace

int main()
{
	using namespace ESCompass;

	const Vec2 north = BearingVector(0.0, 0.0);
	ExpectNear("north x", north.x, 0.0, 1.0e-9);
	ExpectNear("north y", north.y, -1.0, 1.0e-9);

	const Vec2 east = BearingVector(0.0, 90.0);
	ExpectNear("east x", east.x, 1.0, 1.0e-9);
	ExpectNear("east y", east.y, 0.0, 1.0e-9);

	const Vec2 south = BearingVector(0.0, 180.0);
	ExpectNear("south y", south.y, 1.0, 1.0e-9);

	const Vec2 west = BearingVector(0.0, 270.0);
	ExpectNear("west x", west.x, -1.0, 1.0e-9);

	// Wide radar area. True bearings are not equally spaced in pixels.
	const double left = 0.0, top = 0.0, right = 400.0, bottom = 200.0;
	const double cx = 200.0, cy = 100.0;

	const RayHit dueNorth = IntersectBorder(cx, cy, north.x, north.y, left, top, right, bottom);
	Expect(dueNorth.ok, "due north hits");
	ExpectNear("due north x", dueNorth.x, 200.0, 1.0e-6);
	ExpectNear("due north y", dueNorth.y, 0.0, 1.0e-6);

	const double halfHeight = cy - top;
	const RayHit ten = IntersectBorder(cx, cy, BearingVector(0.0, 10.0).x, BearingVector(0.0, 10.0).y, left, top, right, bottom);
	const RayHit twenty = IntersectBorder(cx, cy, BearingVector(0.0, 20.0).x, BearingVector(0.0, 20.0).y, left, top, right, bottom);
	ExpectNear("10 deg x", ten.x, cx + halfHeight * std::tan(DegreesToRadians(10.0)), 1.0e-6);
	ExpectNear("20 deg x", twenty.x, cx + halfHeight * std::tan(DegreesToRadians(20.0)), 1.0e-6);
	const double step10 = ten.x - cx;
	const double step20 = twenty.x - ten.x;
	Expect(std::fabs(step20 - step10) > 1.0, "ticks spread out away from the middle of a long edge");

	const double corner = RadiansToDegrees(std::atan2(right - cx, cy - top));
	const RayHit cornerHit = IntersectBorder(cx, cy, BearingVector(0.0, corner).x, BearingVector(0.0, corner).y, left, top, right, bottom);
	ExpectNear("corner x", cornerHit.x, right, 1.0e-4);
	ExpectNear("corner y", cornerHit.y, top, 1.0e-4);
	Expect(std::fabs(corner - 45.0) > 10.0, "wide-screen corner is not 45 degrees");

	for (int bearing = 0; bearing < 360; ++bearing)
		ExpectBearingHits(0.0, static_cast<double>(bearing), cx, cy, left, top, right, bottom);

	// Geographic north rotated 90 degrees clockwise (points to the right).
	const double turned = 1.5707963267948966;
	const RayHit rotatedNorth = IntersectBorder(cx, cy, BearingVector(turned, 0.0).x, BearingVector(turned, 0.0).y, left, top, right, bottom);
	ExpectNear("rotated 000 x", rotatedNorth.x, right, 1.0e-4);
	ExpectNear("rotated 000 y", rotatedNorth.y, cy, 1.0e-4);

	for (int bearing = 0; bearing < 360; bearing += 7)
		ExpectBearingHits(turned, static_cast<double>(bearing), cx, cy, left, top, right, bottom);

	if (g_Failures != 0)
	{
		std::cerr << g_Failures << " failure(s)\n";
		return 1;
	}

	std::cout << "geometry ok\n";
	return 0;
}
