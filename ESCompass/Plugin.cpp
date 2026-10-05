#include "Plugin.hpp"
#include "RadarScreen.hpp"

#include <string>

namespace
{

std::string NormaliseCommand(const char* text)
{
	std::string out;
	if (!text)
		return out;

	const char* cursor = text;
	while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n')
		++cursor;

	for (; *cursor != '\0'; ++cursor)
	{
		unsigned char character = static_cast<unsigned char>(*cursor);
		if (character == '\r' || character == '\n')
			break;
		if (character >= 'A' && character <= 'Z')
			character = static_cast<unsigned char>(character - 'A' + 'a');
		out.push_back(static_cast<char>(character));
	}

	while (!out.empty() && (out.back() == ' ' || out.back() == '\t'))
		out.pop_back();

	return out;
}

bool ParseToggle(const std::string& line, const char* command, std::string& argument)
{
	const std::string name(command);
	if (line == name)
	{
		argument.clear();
		return true;
	}

	const std::string prefixed = name + " ";
	if (line.compare(0, prefixed.size(), prefixed) != 0)
		return false;

	argument = line.substr(prefixed.size());
	while (!argument.empty() && argument[0] == ' ')
		argument.erase(argument.begin());
	return true;
}

} // namespace

CESCompassPlugin::CESCompassPlugin()
	: EuroScopePlugIn::CPlugIn(
		EuroScopePlugIn::COMPATIBILITY_CODE,
		"ESCompass",
		"1.0.0",
		"George Complin",
		"Copyright (C) 2026 George Complin")
	, m_Rose(false)
	, m_Square(false)
	, m_HaveState(false)
{
}

CESCompassPlugin::~CESCompassPlugin()
{
	for (std::size_t i = 0; i < m_Screens.size(); ++i)
		m_Screens[i]->Detach();
	m_Screens.clear();
}

EuroScopePlugIn::CRadarScreen* CESCompassPlugin::OnRadarScreenCreated(
	const char* sDisplayName,
	bool NeedRadarContent,
	bool GeoReferenced,
	bool CanBeSaved,
	bool CanBeCreated)
{
	(void)sDisplayName;
	(void)NeedRadarContent;
	(void)GeoReferenced;
	(void)CanBeSaved;
	(void)CanBeCreated;

	// Drawn only on screen types allowed in the Plug-ins dialog. Returning a
	// screen for every display is what lets the overlay attach to the
	// standard radar instead of registering a separate view.
	CESCompassScreen* screen = new CESCompassScreen(this);
	m_Screens.push_back(screen);
	return screen;
}

bool CESCompassPlugin::OnCompileCommand(const char* sCommandLine)
{
	const std::string line = NormaliseCommand(sCommandLine);
	std::string argument;
	bool* flag = NULL;
	const char* label = NULL;

	if (ParseToggle(line, ".cmprose", argument) || ParseToggle(line, "cmprose", argument))
	{
		flag = &m_Rose;
		label = "Compass rose";
	}
	else if (ParseToggle(line, ".cmpsquare", argument) || ParseToggle(line, "cmpsquare", argument))
	{
		flag = &m_Square;
		label = "Square compass";
	}
	else
	{
		return false;
	}

	if (argument.empty())
		*flag = !*flag;
	else if (argument == "on")
		*flag = true;
	else if (argument == "off")
		*flag = false;
	else
	{
		Tell("Usage: .cmprose [on|off]   |   .cmpsquare [on|off]");
		return true;
	}

	m_HaveState = true;
	Publish();

	std::string message(label);
	message += *flag ? " ON" : " OFF";
	Tell(message.c_str());
	return true;
}

void CESCompassPlugin::UnregisterScreen(CESCompassScreen* screen)
{
	for (std::vector<CESCompassScreen*>::iterator it = m_Screens.begin(); it != m_Screens.end(); ++it)
	{
		if (*it == screen)
		{
			m_Screens.erase(it);
			return;
		}
	}
}

void CESCompassPlugin::ApplyAsr(const char* rose, const char* square)
{
	if (m_HaveState)
		return;
	if ((rose == NULL || rose[0] == '\0') && (square == NULL || square[0] == '\0'))
		return;

	if (rose != NULL && rose[0] != '\0')
		m_Rose = rose[0] == '1';
	if (square != NULL && square[0] != '\0')
		m_Square = square[0] == '1';
	m_HaveState = true;
}

void CESCompassPlugin::Publish()
{
	for (std::size_t i = 0; i < m_Screens.size(); ++i)
	{
		m_Screens[i]->Persist();
		m_Screens[i]->RequestRefresh();
	}
}

void CESCompassPlugin::Tell(const char* message)
{
	DisplayUserMessage(
		"ESCompass",
		"ESCompass",
		message,
		true,
		true,
		false,
		false,
		false);
}
