#include "EngineSettings.h"

#include "imgui.h"

CCommand::CCommand(const char* inCategory, const char* inCommand, bool inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::BOOL) {
	m_Value.bValue = inDefaultValue;
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

CCommand::CCommand(const char* inCategory, const char* inCommand, int32 inDefaultValue, int32 inMinValue, int32 inMaxValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::INT) {
	m_Value.iValue = {
		.val = inDefaultValue,
		.min = inMinValue,
		.max = inMaxValue
	};
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

CCommand::CCommand(const char* inCategory, const char* inCommand, float inDefaultValue, float inMinValue, float inMaxValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::FLOAT) {
	m_Value.fValue = {
		.val = inDefaultValue,
		.min = inMinValue,
		.max = inMaxValue
	};
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

void CEngineSettings::render() {
	if (ImGui::Begin("Engine Settings")) {
		if (ImGui::BeginTabBar("Settings")) {
			for (auto&[category, commands] : CCommand::getCategoryMap()) {
				if (ImGui::BeginTabItem(category.c_str())) {
					for (auto& command : commands) {
						switch (command->m_Type) {
							case CCommand::Type::BOOL:
								ImGui::Checkbox(command->m_Command, &command->m_Value.bValue);
								break;
							case CCommand::Type::INT:
								ImGui::SliderInt(command->m_Command, &command->m_Value.iValue.val, command->m_Value.iValue.min, command->m_Value.iValue.max);
								break;
							case CCommand::Type::FLOAT:
								ImGui::SliderFloat(command->m_Command, &command->m_Value.fValue.val, command->m_Value.fValue.min, command->m_Value.fValue.max);
								break;
						}
					}
					ImGui::EndTabItem();
				}
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}
