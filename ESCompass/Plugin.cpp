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

// A missing or junk ASR value must not become zero, or a saved screen
// would cancel the default TopSky clearance.
int ParseStoredInset(const char* text)
{
	if (text == NULL || text[0] == '\0')
		return -1;

	int value = 0;
	for (const char* cursor = text; *cursor != '\0'; ++cursor)
	{
		if (*cursor < '0' || *cursor > '9')
			return -1;
		value = value * 10 + (*cursor - '0');
		if (value > 400)
			return -1;
	}
	return value;
}

bool ParseInsetArgument(const std::string& argument, int& value)
{
	if (argument == "off")
	{
		value = 0;
		return true;
	}
	value = ParseStoredInset(argument.c_str());
	return value >= 0;
}

} // namespace

CESCompassPlugin::CESCompassPlugin()
	: EuroScopePlugIn::CPlugIn(
		EuroScopePlugIn::COMPATIBILITY_CODE,
		"ESCompass",
		"1.1.0",
		"George Complin",
		"Copyright (C) 2026 George Complin")
	, m_Rose(false)
	, m_Square(false)
	, m_TopInset(20)
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
	else if (ParseToggle(line, ".cmptop", argument) || ParseToggle(line, "cmptop", argument))
	{
		int inset = 0;
		if (argument.empty())
		{
			std::string message = "Top clearance ";
			message += std::to_string(m_TopInset);
			message += " px";
			Tell(message.c_str());
			return true;
		}
		if (!ParseInsetArgument(argument, inset))
		{
			Tell("Usage: .cmptop <pixels>   (0 draws to the top edge)");
			return true;
		}

		m_TopInset = inset;
		m_HaveState = true;
		Publish();

		std::string message = "Top clearance ";
		message += std::to_string(m_TopInset);
		message += " px";
		Tell(message.c_str());
		return true;
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

void CESCompassPlugin::ApplyAsr(const char* rose, const char* square, const char* top)
{
	if (m_HaveState)
		return;

	const bool haveRose = rose != NULL && rose[0] != '\0';
	const bool haveSquare = square != NULL && square[0] != '\0';
	const int inset = ParseStoredInset(top);
	if (!haveRose && !haveSquare && inset < 0)
		return;

	if (haveRose)
		m_Rose = rose[0] == '1';
	if (haveSquare)
		m_Square = square[0] == '1';
	if (inset >= 0)
		m_TopInset = inset;
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
