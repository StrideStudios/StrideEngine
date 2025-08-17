#include "EngineSettings.h"

#include "imgui.h"
#include "tracy/Tracy.hpp"

CEngineSettings& CEngineSettings::get() {
	static CEngineSettings sEngineSettings;
	return sEngineSettings;
}

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