#pragma once

#include "Plugin.hpp"

class CESCompassScreen : public EuroScopePlugIn::CRadarScreen
{
public:
	explicit CESCompassScreen(CESCompassPlugin* plugin);
	virtual ~CESCompassScreen();

	inline virtual void OnAsrContentToBeClosed(void)
	{
		delete this;
	}

	virtual void OnRefresh(HDC hDC, int Phase);
	virtual void OnAsrContentLoaded(bool Loaded);
	virtual void OnAsrContentToBeSaved(void);

	void Persist();
	void Detach();

private:
	RECT DrawableRadarArea();
	double GeographicNorthAngle(const RECT& area);

	CESCompassPlugin* m_Plugin;
};
