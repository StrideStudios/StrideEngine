#include "EngineSettings.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

void CEngineSettings::render() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("Engine Settings")) {
		if (ImGui::BeginTabBar("Settings")) {
			for (auto&[category, commands] : CCommand::getCategoryMap()) {
				if (ImGui::BeginTabItem(category.c_str())) {
					for (auto& command : commands) {
						command->render();
					}
					ImGui::EndTabItem();
				}
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	ImGui::Render();
}
