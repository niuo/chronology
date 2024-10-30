#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class chronology : public BakkesMod::Plugin::BakkesModPlugin ,public SettingsWindowBase
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

	//Boilerplate
	void onLoad() override;
	void Render(CanvasWrapper canvas);
	void scoreboardLoad(std::string eventName);
	void scoreboardClose(std::string eventName);
	void onStatTickerMessage(void* params);
	void loadMenu(std::string eventName);
	//void onUnload() override; // Uncomment and implement if you need a unload method

	//images
	std::shared_ptr<ImageWrapper> imgGround;
	std::shared_ptr<ImageWrapper> imgGoal;
	std::shared_ptr<ImageWrapper> imgEpicSave;
	std::shared_ptr<ImageWrapper> imgRound;

	//size
	int barTotalWidth = 800;
	int barTotalHeight = 15;

	//events
	int globalTime = 0;
	int currentTime = 0;
	std::vector < std::tuple<int, std::string, int, std::string> > events; //time, event, team, player

	//display position
	float xOffset = 27;
	float yOffset = 18;

	// The structure of a ticker stat event
	struct StatTickerParams {
		uintptr_t Receiver; // person who got a stat
		uintptr_t Victim; // person who is victim of a stat (only exists for demos afaik)
		uintptr_t StatEvent; // wrapper for the stat event
	};

	// structure of a stat event
	struct StatEventParams {
		uintptr_t PRI; // always primary player
		uintptr_t StatEvent; // wrapper for the stat event
	};

public:
	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
