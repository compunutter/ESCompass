#include "RadarScreen.hpp"
#include "Geometry.hpp"

#define NOMINMAX
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace
{

const Gdiplus::Color kCompassColour(255, 154, 192, 159);

double TickLength(int bearing)
{
	if (bearing % 90 == 0)
		return 24.0;
	if (bearing % 10 == 0)
		return 18.0;
	if (bearing % 5 == 0)
		return 11.0;
	return 5.0;
}

void FormatBearing(int bearing, wchar_t text[4])
{
	bearing %= 360;
	if (bearing < 0)
		bearing += 360;
	text[0] = static_cast<wchar_t>(L'0' + (bearing / 100) % 10);
	text[1] = static_cast<wchar_t>(L'0' + (bearing / 10) % 10);
	text[2] = static_cast<wchar_t>(L'0' + bearing % 10);
	text[3] = 0;
}

void DrawCentered(
	Gdiplus::Graphics& graphics,
	const Gdiplus::Font& font,
	const Gdiplus::Brush& brush,
	const wchar_t* text,
	double x,
	double y)
{
	Gdiplus::RectF bounds;
	graphics.MeasureString(text, -1, &font, Gdiplus::PointF(0.0f, 0.0f), &bounds);
	graphics.DrawString(
		text,
		-1,
		&font,
		Gdiplus::PointF(
			static_cast<Gdiplus::REAL>(x - bounds.Width / 2.0),
			static_cast<Gdiplus::REAL>(y - bounds.Height / 2.0)),
		&brush);
}

void DrawTick(
	Gdiplus::Graphics& graphics,
	Gdiplus::Pen& pen,
	double cx,
	double cy,
	double vx,
	double vy,
	double outer,
	double length)
{
	const double inner = (std::max)(0.0, outer - length);
	graphics.DrawLine(
		&pen,
		static_cast<Gdiplus::REAL>(cx + vx * outer),
		static_cast<Gdiplus::REAL>(cy + vy * outer),
		static_cast<Gdiplus::REAL>(cx + vx * inner),
		static_cast<Gdiplus::REAL>(cy + vy * inner));
}

bool ValidRect(const RECT& rect)
{
	return rect.right > rect.left + 1 && rect.bottom > rect.top + 1;
}

bool Overlaps(const RECT& a, const RECT& b)
{
	return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
}

} // namespace

CESCompassScreen::CESCompassScreen(CESCompassPlugin* plugin)
	: m_Plugin(plugin)
{
}

CESCompassScreen::~CESCompassScreen()
{
	if (m_Plugin != NULL)
		m_Plugin->UnregisterScreen(this);
}

void CESCompassScreen::OnAsrContentLoaded(bool Loaded)
{
	(void)Loaded;
	const char* rose = GetDataFromAsr("ESCompassRose");
	const char* square = GetDataFromAsr("ESCompassSquare");
	const char* top = GetDataFromAsr("ESCompassTop");
	if (m_Plugin != NULL)
		m_Plugin->ApplyAsr(rose, square, top);
}

void CESCompassScreen::OnAsrContentToBeSaved(void)
{
	Persist();
}

void CESCompassScreen::Detach()
{
	m_Plugin = NULL;
}

void CESCompassScreen::Persist()
{
	if (m_Plugin == NULL)
		return;

	SaveDataToAsr(
		"ESCompassRose",
		"ESCompass compass rose",
		m_Plugin->Rose() ? "1" : "0");
	SaveDataToAsr(
		"ESCompassSquare",
		"ESCompass square compass",
		m_Plugin->Square() ? "1" : "0");

	const std::string top = std::to_string(m_Plugin->TopInset());
	SaveDataToAsr(
		"ESCompassTop",
		"ESCompass top clearance",
		top.c_str());
}

RECT CESCompassScreen::DrawableRadarArea()
{
	RECT radar = GetRadarArea();
	const RECT original = radar;
	const RECT chat = GetChatArea();
	const RECT toolbar = GetToolbarArea();

	if (ValidRect(chat) && Overlaps(radar, chat))
	{
		const int midY = (radar.top + radar.bottom) / 2;
		if (chat.top >= midY && chat.top < radar.bottom)
			radar.bottom = chat.top;
		else if (chat.bottom <= midY && chat.bottom > radar.top)
			radar.top = chat.bottom;
	}

	if (ValidRect(toolbar) && Overlaps(radar, toolbar))
	{
		const int midY = (radar.top + radar.bottom) / 2;
		const int midX = (radar.left + radar.right) / 2;
		const bool wide = (toolbar.right - toolbar.left) > (original.right - original.left) / 2;
		const bool tall = (toolbar.bottom - toolbar.top) > (original.bottom - original.top) / 2;

		if (wide && toolbar.bottom <= midY && toolbar.bottom > radar.top)
			radar.top = toolbar.bottom;
		else if (wide && toolbar.top >= midY && toolbar.top < radar.bottom)
			radar.bottom = toolbar.top;
		else if (tall && toolbar.right <= midX && toolbar.right > radar.left)
			radar.left = toolbar.right;
		else if (tall && toolbar.left >= midX && toolbar.left < radar.right)
			radar.right = toolbar.left;
	}

	if (!ValidRect(radar))
		return original;

	const int originalHeight = original.bottom - original.top;
	const int clippedHeight = radar.bottom - radar.top;
	if (originalHeight > 0 && clippedHeight * 3 < originalHeight)
		return original;

	// TopSky draws its global menu over the top of the radar after other
	// plugins, and does not report that strip as a toolbar. Keep both
	// compasses below it. .cmptop changes the depth.
	if (m_Plugin != NULL)
	{
		int inset = m_Plugin->TopInset();
		if (inset < 0)
			inset = 0;
		if (inset > 400)
			inset = 400;
		if (inset > 0 && radar.bottom - radar.top > inset + 40)
			radar.top += inset;
	}

	return radar;
}

double CESCompassScreen::GeographicNorthAngle(const RECT& area)
{
	POINT origin;
	origin.x = (area.left + area.right) / 2;
	origin.y = (area.top + area.bottom) / 2;

	const EuroScopePlugIn::CPosition here = ConvertCoordFromPixelToPosition(origin);
	double step = 0.02;

	for (int attempt = 0; attempt < 6; ++attempt)
	{
		EuroScopePlugIn::CPosition there;
		there.m_Longitude = here.m_Longitude;
		there.m_Latitude = here.m_Latitude + step;

		bool flipped = false;
		if (there.m_Latitude > 89.0)
		{
			there.m_Latitude = here.m_Latitude - step;
			flipped = true;
		}

		const POINT pixel = ConvertCoordFromPositionToPixel(there);
		double dx = static_cast<double>(pixel.x - origin.x);
		double dy = static_cast<double>(pixel.y - origin.y);
		if (flipped)
		{
			dx = -dx;
			dy = -dy;
		}

		if (std::hypot(dx, dy) >= 2.0)
			return ESCompass::ScreenAngle(dx, dy);

		step *= 3.0;
	}

	return 0.0;
}

void CESCompassScreen::OnRefresh(HDC hDC, int Phase)
{
	if (Phase != EuroScopePlugIn::REFRESH_PHASE_BEFORE_TAGS)
		return;
	if (m_Plugin == NULL || (!m_Plugin->Rose() && !m_Plugin->Square()))
		return;

	const RECT area = DrawableRadarArea();
	const double left = static_cast<double>(area.left);
	const double top = static_cast<double>(area.top);
	const double right = static_cast<double>(area.right);
	const double bottom = static_cast<double>(area.bottom);
	const double width = right - left;
	const double height = bottom - top;
	if (width < 20.0 || height < 20.0)
		return;

	const double cx = static_cast<double>((area.left + area.right) / 2);
	const double cy = static_cast<double>((area.top + area.bottom) / 2);
	const double north = GeographicNorthAngle(area);

	Gdiplus::Graphics* graphics = Gdiplus::Graphics::FromHDC(hDC);
	if (graphics == NULL)
		return;

	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
	graphics->SetClip(Gdiplus::Rect(
		area.left,
		area.top,
		static_cast<INT>(width),
		static_cast<INT>(height)));

	Gdiplus::Pen minorPen(kCompassColour, 1.0f);
	Gdiplus::Pen majorPen(kCompassColour, 1.7f);
	Gdiplus::SolidBrush textBrush(kCompassColour);
	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font font(&fontFamily, 8.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
	const bool drawText = fontFamily.GetLastStatus() == Gdiplus::Ok && font.GetLastStatus() == Gdiplus::Ok;

	if (m_Plugin->Square())
	{
		for (int bearing = 0; bearing < 360; ++bearing)
		{
			const ESCompass::Vec2 vector = ESCompass::BearingVector(north, static_cast<double>(bearing));
			const ESCompass::RayHit hit = ESCompass::IntersectBorder(
				cx, cy, vector.x, vector.y, left, top, right, bottom);
			if (!hit.ok)
				continue;

			const double length = TickLength(bearing);
			Gdiplus::Pen& pen = (bearing % 10 == 0) ? majorPen : minorPen;
			DrawTick(*graphics, pen, cx, cy, vector.x, vector.y, hit.t, length);

			if (!drawText || bearing % 10 != 0)
				continue;

			wchar_t label[4];
			FormatBearing(bearing, label);
			double inset = length + 12.0;
			for (int nudge = 0; nudge < 6; ++nudge)
			{
				const double distance = hit.t - inset;
				if (distance < 20.0)
					break;

				const double x = cx + vector.x * distance;
				const double y = cy + vector.y * distance;
				if (x > left + 18.0 && x < right - 18.0 && y > top + 12.0 && y < bottom - 12.0)
				{
					DrawCentered(*graphics, font, textBrush, label, x, y);
					break;
				}
				inset += 8.0;
			}
		}
	}

	if (m_Plugin->Rose())
	{
		const double radius = (std::min)(width, height) / 2.0 - 2.0;
		if (radius > 12.0)
		{
			graphics->DrawEllipse(
				&majorPen,
				static_cast<Gdiplus::REAL>(cx - radius),
				static_cast<Gdiplus::REAL>(cy - radius),
				static_cast<Gdiplus::REAL>(radius * 2.0),
				static_cast<Gdiplus::REAL>(radius * 2.0));

			for (int bearing = 0; bearing < 360; ++bearing)
			{
				const ESCompass::Vec2 vector = ESCompass::BearingVector(north, static_cast<double>(bearing));
				const double length = TickLength(bearing);
				Gdiplus::Pen& pen = (bearing % 10 == 0) ? majorPen : minorPen;
				DrawTick(*graphics, pen, cx, cy, vector.x, vector.y, radius, length);

				if (!drawText || bearing % 10 != 0)
					continue;

				const double labelRadius = radius - length - 12.0;
				if (labelRadius < 16.0)
					continue;

				wchar_t label[4];
				FormatBearing(bearing, label);
				DrawCentered(
					*graphics,
					font,
					textBrush,
					label,
					cx + vector.x * labelRadius,
					cy + vector.y * labelRadius);
			}
		}
	}

	delete graphics;
}
