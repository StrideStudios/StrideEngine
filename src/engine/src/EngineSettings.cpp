#include "EngineSettings.h"

#include "imgui.h"
#include "Profiling.h"
#include "tracy/Tracy.hpp"

CCommand* CEngineSettings::getCommand(const std::string& inCommand) {

	CCommand* outCommand = nullptr;
	for (const auto&[command, commandType] : get().m_Commands) {
		if (command == inCommand) {
			outCommand = commandType;
			break;
		}
	}

	return outCommand;
}

void CEngineSettings::addCommand(const std::string& inCommand, CCommand* inCommandType) {
	//ast(!get().m_Commands.contains(inCommand), "Command {} already defined.", inCommand);
	get().m_Commands.emplace_back(inCommand, inCommandType);
}

std::vector<std::pair<std::string, std::vector<CGuiType*>>>& CGuiType::getCategoryMap() {
	static std::vector<std::pair<std::string, std::vector<CGuiType*>>> map;
	return map;
}

void CGuiText::render() {
	ImGui::Text(m_Text.c_str());
}

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

void CommandRenderType::checkbox(const char* inCommand, bool* outValue) {
	ImGui::Checkbox(inCommand, outValue);
}

void CommandRenderType::intSlider(const char* inCommand, int32* outValue, const int32 min, const int32 max) {
	ImGui::SliderInt(inCommand, outValue, min, max);
}

void CommandRenderType::floatSlider(const char* inCommand, float* outValue, const float min, const float max) {
	ImGui::SliderFloat(inCommand, outValue, min, max);
}

void CommandRenderType::int2Input(const char* inCommand, int32* outValue) {
	ImGui::InputInt2(inCommand, outValue);
}

void CommandRenderType::float2Input(const char* inCommand, float* outValue) {
	ImGui::InputFloat2(inCommand, outValue);

}

void CommandRenderType::float3Input(const char* inCommand, float* outValue) {
	ImGui::InputFloat3(inCommand, outValue);
}

void CommandRenderType::float4Input(const char* inCommand, float* outValue) {
	ImGui::InputFloat4(inCommand, outValue);
}
