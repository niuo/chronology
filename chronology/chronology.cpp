#include "pch.h"
#include "chronology.h"


BAKKESMOD_PLUGIN(chronology, "Dsiplay the timeline of each goal during a game", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
bool chronologyEnabled = true;


void chronology::onLoad() {
	_globalCvarManager = cvarManager;
	
	cvarManager->registerCvar("chronology_enabled", "1", "Enable Chronology plugin", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		chronologyEnabled = cvar.getBoolValue();
			});

	imgGround = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "chronology" / "chronology_ground.png", true, false);
	imgGoal = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "chronology" / "chronology_goal.png", true, false);
	imgEpicSave = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "chronology" / "chronology_epicsave.png", true, false);

	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", bind(&chronology::scoreboardLoad, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", bind(&chronology::scoreboardClose, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", bind(&chronology::loadMenu, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.WaitingForPlayers.EndState", bind(&chronology::loadMenu, this, std::placeholders::_1));

	//  We need the params so we hook with caller, but there is no wrapper for the HUD
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			onStatTickerMessage(params);
		});
	
	//DEBUG - go freeplay
	//blueGoals.push_back(5);
	//orangeGoals.push_back(233);
}


void chronology::Render(CanvasWrapper canvas) {
	if (!chronologyEnabled) { return; }
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { return; }
	if (!gameWrapper->IsInGame()) { return; }
	
	// timer
	if (!server.GetbOverTime()) currentTime = server.GetSecondsRemaining();
	globalTime = server.GetGameTime();
		
	//full timeline
	if (imgGround->IsLoadedForCanvas()) {
		canvas.SetColor(LinearColor(100, 120, 123, 255));
		canvas.SetPosition(Vector2F{ xOffset , yOffset });
		canvas.DrawRect(imgGroundWidth, 10, imgGround.get());

		float width = imgGroundWidth * (globalTime - currentTime) / globalTime;
		canvas.SetColor(LinearColor(214, 236, 239, 255));
		canvas.SetPosition(Vector2F{ xOffset + width, yOffset - 2 });
		canvas.DrawString(std::format("{}:{:02}", (int)floor(currentTime / 60), (int)(currentTime % 60)));
		canvas.SetPosition(Vector2F{ xOffset , yOffset });
		canvas.DrawRect(width, 10, imgGround.get());
	}

	//all events
	for (auto [a, b, c, d] : events) { //time, event, team, player
		LOG("[TICKER] time:{}, name:{}, team:{}, player:{}", a, b, c, d);
		float xPos = imgGroundWidth * (globalTime - a) / globalTime;
		//background time
		if (c == 0) canvas.SetColor(LinearColor(100, 100, 255, 100));
		else canvas.SetColor(LinearColor(255, 165, 0, 100));
		canvas.SetPosition(Vector2F{ xOffset - 20 + xPos , yOffset + 15 });
		canvas.DrawRect(40, 15, imgGround.get());
		//time of the event
		canvas.SetColor(LinearColor(200, 200, 200, 255));
		canvas.SetPosition(Vector2F{ xOffset - 15 + xPos , yOffset + 15 });
		canvas.DrawString(std::format("{}:{:02}", (int)floor(a / 60), (int)(a % 60)));
		//icon of the event
		if (c == 0) canvas.SetColor(LinearColor(100, 100, 255, 255));
		else canvas.SetColor(LinearColor(255, 165, 0, 255));
		canvas.DrawLine(Vector2F{ xOffset + xPos, yOffset + 10 }, Vector2F{ xOffset + xPos, yOffset - 1 }, 2);
		canvas.SetPosition(Vector2F{ xOffset - 14 + xPos , yOffset - 30 });
		if ((b == "Goal") && (imgGoal->IsLoadedForCanvas())) canvas.DrawTexture(imgGoal.get(), 0.5);
		if ((b == "EpicSave") && (imgEpicSave->IsLoadedForCanvas())) canvas.DrawTexture(imgEpicSave.get(), 0.5);
		//name of the player
		canvas.SetPosition(Vector2F{ xOffset - (d.length()*2) + xPos , yOffset - 40});
		canvas.DrawString(d, 1, 1, true, false);
	}



}

void chronology::scoreboardLoad(std::string eventName) {
	Vector2 screenSize = gameWrapper->GetScreenSize();
	(imgGroundWidth > screenSize.X) ? screenSize.X : 800;

	xOffset = (((float)screenSize.X - imgGroundWidth) / 2);
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

	// reverse timer
	//if (sw.GetSecondsRemaining() == 300) time = 0;
	//else if (300 - sw.GetSecondsRemaining() > time) time = 300 - sw.GetSecondsRemaining();

	if (statEvent.GetEventName() == "EpicSave") {
		if (!receiver) { LOG("Null reciever PRI"); return; }
		events.push_back(std::tuple<int, std::string, int, std::string>(currentTime, statEvent.GetEventName(), receiver.GetTeam().GetTeamIndex(), receiver.GetPlayerName().ToString()));
	}
	if (statEvent.GetEventName() == "Goal") {
		if (!receiver) { LOG("Null reciever PRI"); return; }
		events.push_back(std::tuple<int, std::string, int, std::string>(currentTime, statEvent.GetEventName(), receiver.GetTeam().GetTeamIndex(), receiver.GetPlayerName().ToString()));
	}
	
}


void chronology::loadMenu(std::string eventName) {
	events.clear();
	currentTime = 0;
	globalTime = 0;
}