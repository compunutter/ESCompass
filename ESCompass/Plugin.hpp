#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <EuroScopePlugIn.h>

#include <vector>

class CESCompassScreen;

class CESCompassPlugin : public EuroScopePlugIn::CPlugIn
{
public:
	CESCompassPlugin();
	virtual ~CESCompassPlugin();

	virtual EuroScopePlugIn::CRadarScreen* OnRadarScreenCreated(
		const char* sDisplayName,
		bool NeedRadarContent,
		bool GeoReferenced,
		bool CanBeSaved,
		bool CanBeCreated);

	virtual bool OnCompileCommand(const char* sCommandLine);

	bool Rose() const { return m_Rose; }
	bool Square() const { return m_Square; }

	void UnregisterScreen(CESCompassScreen* screen);
	void ApplyAsr(const char* rose, const char* square);

private:
	void Publish();
	void Tell(const char* message);

	bool m_Rose;
	bool m_Square;
	bool m_HaveState;
	std::vector<CESCompassScreen*> m_Screens;
};
