#include "EngineSettings.h"

#include "VulkanDevice.h"
#include "imgui.h"
#include "tracy/Tracy.hpp"

void CEngineSettings::render() {
	ZoneScopedN("Engine UI");

	if (ImGui::Begin("Engine Settings")) {
		if (ImGui::BeginTabBar("Settings")) {
			for (auto&[category, guiTypes] : CGuiType::getCategoryMap()) {
				if (ImGui::BeginTabItem(category.c_str())) {
					for (auto& command : guiTypes) {
						command->render();
					}
					ImGui::EndTabItem();
				}
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}