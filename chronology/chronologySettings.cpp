#include "pch.h"
#include "chronology.h"

void chronology::RenderSettings() {
    ImGui::TextUnformatted("This plugin only works in game");

    CVarWrapper enableCvar = cvarManager->getCvar("chronology_enabled");
    if (!enableCvar) { return; }
    bool enabled = enableCvar.getBoolValue();
    if (ImGui::Checkbox("Enable plugin", &enabled)) {
        enableCvar.setValue(enabled);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Chronology Plugin");
    }
}