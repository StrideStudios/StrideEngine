#include "EngineSettings.h"

#include "imgui.h"

CCommand::CCommand(const char* inCategory, const char* inCommand, const bool inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::BOOL) {
	m_Value.bValue = inDefaultValue;
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

CCommand::CCommand(const char* inCategory, const char* inCommand, const int32 inDefaultValue, int32 inMinValue, int32 inMaxValue)
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

CCommand::CCommand(const char* inCategory, const char* inCommand, const float inDefaultValue, float inMinValue, float inMaxValue)
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

CCommand::CCommand(const char* inCategory, const char* inCommand, const Extent32 inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::EXTENT) {
	m_Value.extent = inDefaultValue;
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

CCommand::CCommand(const char* inCategory, const char* inCommand, const Vector2f inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::VECTOR2) {
	m_Value.vector2 = inDefaultValue;
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

CCommand::CCommand(const char* inCategory, const char* inCommand, const Vector3f inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::VECTOR3) {
	m_Value.vector3 = inDefaultValue;
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap();
}

CCommand::CCommand(const char* inCategory, const char* inCommand, const Vector4f inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::VECTOR4) {
	m_Value.vector4 = inDefaultValue;
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
							case CCommand::Type::EXTENT:
								ImGui::InputInt2(command->m_Command, (int32*)&command->m_Value.extent);
								break;
							case CCommand::Type::VECTOR2:
								ImGui::InputFloat2(command->m_Command, (float*)&command->m_Value.vector2);
								break;
							case CCommand::Type::VECTOR3:
								ImGui::InputFloat3(command->m_Command, (float*)&command->m_Value.vector3);
								break;
							case CCommand::Type::VECTOR4:
								ImGui::InputFloat4(command->m_Command, (float*)&command->m_Value.vector4);
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
