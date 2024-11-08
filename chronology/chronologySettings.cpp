#include "pch.h"
#include "chronology.h"

void chronology::RenderSettings() {
    ImGui::TextUnformatted("This plugin only works in game");

    // boolean enable/disable
    CVarWrapper enableCvar = cvarManager->getCvar("chronology_enabled");
    if (!enableCvar) { return; }
    bool enabled = enableCvar.getBoolValue();
    if (ImGui::Checkbox("Enable plugin", &enabled)) {
        enableCvar.setValue(enabled);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Chronology Plugin");
    }

    // slider for the width
    CVarWrapper widthCvar = cvarManager->getCvar("chronology_barwidth");
    if (!widthCvar) { return; }
    float width = widthCvar.getFloatValue();
    if (ImGui::SliderFloat("% of screen width used for the timeline (default:75%)", &width, 10.0, 90.0, "%.0f %%")) {
        widthCvar.setValue(width);
    }

    // slider for the Y offset
    CVarWrapper offsetYCvar = cvarManager->getCvar("chronology_offsetY");
    if (!offsetYCvar) { return; }
    float offsetY = offsetYCvar.getFloatValue();
    if (ImGui::SliderFloat("screen heigth position (default:20%)", &offsetY, 0.0, 100.0, "%.0f %%")) {
        offsetYCvar.setValue(offsetY);
    }
}