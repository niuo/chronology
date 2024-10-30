#include "pch.h"
#include "chronology.h"


BAKKESMOD_PLUGIN(chronology, "Dsiplay an event timeline of the game", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
bool chronologyEnabled = true;


void chronology::onLoad() {
	_globalCvarManager = cvarManager;
	
	cvarManager->registerCvar("chronology_enabled", "1", "Enable Chronology plugin", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		chronologyEnabled = cvar.getBoolValue();
			});

	imgGoal = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "chronology" / "chronology_goal.png", true, false);
	imgEpicSave = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "chronology" / "chronology_epicsave.png", true, false);
	imgRound = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "chronology" / "chronology_round.png", true, false);

	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", bind(&chronology::scoreboardLoad, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", bind(&chronology::scoreboardClose, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", bind(&chronology::loadMenu, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.WaitingForPlayers.EndState", bind(&chronology::loadMenu, this, std::placeholders::_1));

	//  We need the params so we hook with caller, but there is no wrapper for the HUD
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			onStatTickerMessage(params);
		});
		
	//debug
	/**/
	events.push_back(std::tuple<int, std::string, int, std::string>(250, "Goal", 0, "niuognip"));
	events.push_back(std::tuple<int, std::string, int, std::string>(200, "Goal", 1, "bufalo"));
	events.push_back(std::tuple<int, std::string, int, std::string>(195, "EpicSave", 0, "tri"));
	events.push_back(std::tuple<int, std::string, int, std::string>(150, "EpicSave", 1, "cacatoes"));
	events.push_back(std::tuple<int, std::string, int, std::string>(100, "Goal", 1, "niuognip"));
	events.push_back(std::tuple<int, std::string, int, std::string>(90, "Goal", 1, "bufalo"));
	events.push_back(std::tuple<int, std::string, int, std::string>(80, "Goal", 1, "tri"));
	events.push_back(std::tuple<int, std::string, int, std::string>(60, "EpicSave", 1, "cacatoes"));
	
}

//TODO
// pouvroi choisir longeur de la barwidth et de la barheight



void chronology::Render(CanvasWrapper canvas) {
	if (!chronologyEnabled) { return; }
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { return; }
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame()) { return; }
	
	// timer
	if (!server.GetbOverTime()) currentTime = server.GetSecondsRemaining();
	if (server.GetbOverTime()) currentTime = 0;
	globalTime = server.GetGameTime();
	int game60sec = 60 * barTotalWidth / globalTime;
	int game30sec = 30 * barTotalWidth / globalTime;

	// background bar
	canvas.SetColor(LinearColor(255, 255, 255, 100));
	canvas.DrawRect(Vector2F{ xOffset, yOffset}, Vector2F{ xOffset + barTotalWidth, yOffset + barTotalHeight });
	
	// timezone
	canvas.SetColor(LinearColor(0, 255, 0, 150)); // green
	canvas.DrawRect(Vector2F{ xOffset, yOffset + barTotalHeight }, Vector2F{ xOffset + barTotalWidth - game60sec, yOffset + barTotalHeight + 3 });
	canvas.SetPosition(Vector2F{ xOffset - (canvas.GetStringSize("00:00").X / 2) , yOffset + barTotalHeight + 3 });
	canvas.DrawString(std::format("{}:{:02}", (int)floor(globalTime / 60), (int)(globalTime % 60)));
	canvas.SetColor(LinearColor(0, 150, 0, 50)); 
	canvas.DrawRect(Vector2F{ xOffset - (canvas.GetStringSize("00:00").X/2) , yOffset + barTotalHeight }, Vector2F{ xOffset + (canvas.GetStringSize("00:00").X/2) , yOffset + barTotalHeight + canvas.GetStringSize("00:00").Y });
	canvas.SetColor(LinearColor(255, 255, 0, 150)); // orange
	canvas.DrawRect(Vector2F{ xOffset + barTotalWidth - game60sec, yOffset + barTotalHeight }, Vector2F{ xOffset + barTotalWidth - game60sec + game30sec, yOffset + barTotalHeight + 2 });
	canvas.SetPosition(Vector2F{ xOffset + barTotalWidth - game60sec - (canvas.GetStringSize("01:00").X / 2) , yOffset + barTotalHeight + 3 });
	canvas.DrawString("01:00");
	canvas.SetColor(LinearColor(255, 0, 0, 150)); // red
	canvas.DrawRect(Vector2F{ xOffset + barTotalWidth - game30sec, yOffset + barTotalHeight }, Vector2F{ xOffset + barTotalWidth, yOffset + barTotalHeight + 2 });
	canvas.SetPosition(Vector2F{ xOffset + barTotalWidth - game30sec - (canvas.GetStringSize("00:30").X/2) , yOffset + barTotalHeight + 3});
	canvas.DrawString("00:30");

	//loading + timer
	float width = barTotalWidth * (globalTime - currentTime) / globalTime;
	canvas.SetColor(LinearColor(0, 0, 0, 255));
	canvas.SetPosition(Vector2F{ xOffset + width + 2, yOffset });
	canvas.DrawString(std::format("{}:{:02}", (int)floor(currentTime / 60), (int)(currentTime % 60)));
	canvas.SetPosition(Vector2F{ xOffset, yOffset });
	canvas.SetColor(LinearColor(0, 0, 0, 160));
	canvas.DrawRect(Vector2F{ xOffset, yOffset }, Vector2F{ xOffset + width, yOffset + barTotalHeight + 3 });
	
	//all events
	int num = 0;
	for (auto [a, b, c, d] : events) { //time, event, team, player
		float xPos = barTotalWidth * (globalTime - a) / globalTime;
		float yPos = num += 40; //change height dynamically
		if (num > (40*1)) num = 0;
		if (d.length() >= 6) {
			d.resize(5);
			d = d + ".";
		}

		//background time
		if (c == 0) canvas.SetColor(LinearColor(100, 100, 255, 100));
		else canvas.SetColor(LinearColor(255, 165, 0, 100));
		//canvas.SetPosition(Vector2F{ xOffset - 20 + xPos , yOffset + 15 });
		//canvas.DrawRect(40, 15, imgGround.get());
		
		//time of the event
		//canvas.SetColor(LinearColor(255, 255, 255, 255));
		//canvas.SetPosition(Vector2F{ xOffset - 15 + xPos , yOffset + 15 });
		//canvas.DrawString(std::format("{}:{:02}", (int)floor(a / 60), (int)(a % 60)), 0.8, 0.8);
		canvas.FillTriangle(
			Vector2F{ xOffset - 15 + xPos , yOffset + 35 },
			Vector2F{ xOffset + xPos , yOffset + 35 },
			Vector2F{ xOffset - 5 + xPos , yOffset + 55 },
			LinearColor(255, 255, 255, 255));

		//draw round
		if (c == 0 && b == "Goal") canvas.SetColor(LinearColor(100, 100, 255, 255));
		else if (c == 1 && b == "Goal") canvas.SetColor(LinearColor(255, 165, 0, 255));
		else if (c == 0 && b == "EpicSave") canvas.SetColor(LinearColor(20, 20, 255, 255));
		else if (c == 1 && b == "EpicSave") canvas.SetColor(LinearColor(255, 50, 0, 255));
		canvas.DrawLine(Vector2F{ xOffset + xPos, yOffset - yPos }, Vector2F{ xOffset + xPos, yOffset + barTotalHeight }, 4);
		canvas.SetPosition(Vector2F{ xOffset + xPos - 20 , yOffset - yPos });
		canvas.DrawTexture(imgRound.get(), 0.5);
		canvas.SetPosition(Vector2F{ xOffset + xPos - 16 , yOffset - yPos + 4 });
		canvas.SetColor(LinearColor(255, 255, 255, 255));
		if ((b == "Goal") && (imgGoal->IsLoadedForCanvas())) canvas.DrawTexture(imgGoal.get(), 0.5);
		if ((b == "EpicSave") && (imgEpicSave->IsLoadedForCanvas())) canvas.DrawTexture(imgEpicSave.get(), 0.5);
		canvas.SetPosition(Vector2F{ xOffset + xPos + 20 , yOffset - yPos + 15 });
		canvas.DrawString(d, 1, 1, true, false);
	}

}

void chronology::scoreboardLoad(std::string eventName) {
	Vector2 screenSize = gameWrapper->GetScreenSize();
	(barTotalWidth > screenSize.X) ? screenSize.X : 800;

	xOffset = (((float)screenSize.X - barTotalWidth) / 2);
	yOffset = (float)screenSize.Y / 5;

	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		Render(canvas);
		});
}

void chronology::scoreboardClose(std::string eventName) {
	gameWrapper->UnregisterDrawables();
}


void chronology::onStatTickerMessage(void* params) {
	StatTickerParams* pStruct = (StatTickerParams*)params;
	PriWrapper receiver = PriWrapper(pStruct->Receiver);
	PriWrapper victim = PriWrapper(pStruct->Victim);
	StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);

	ServerWrapper sw = gameWrapper->GetCurrentGameState();
	if (!sw) { return; }
	if (sw.GetbOverTime()) { return; }
	currentTime = sw.GetSecondsRemaining();

	if (statEvent.GetEventName() == "EpicSave") {
		if (!receiver) { LOG("Chronology: Null reciever PRI"); return; }
		events.push_back(std::tuple<int, std::string, int, std::string>(currentTime, statEvent.GetEventName(), receiver.GetTeam().GetTeamIndex(), receiver.GetPlayerName().ToString()));
	}
	if (statEvent.GetEventName() == "Goal") {
		if (!receiver) { LOG("Chronology: Null reciever PRI"); return; }
		events.push_back(std::tuple<int, std::string, int, std::string>(currentTime, statEvent.GetEventName(), receiver.GetTeam().GetTeamIndex(), receiver.GetPlayerName().ToString()));
	}
	
}


void chronology::loadMenu(std::string eventName) {
	events.clear();
	currentTime = 0;
	globalTime = 0;
}