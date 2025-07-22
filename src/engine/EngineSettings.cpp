#include "EngineSettings.h"

CCommand::CCommand(const char* inCategory, const char* inCommand, bool inDefaultValue)
: m_Category(inCategory),
m_Command(inCommand),
m_Type(Type::BOOL) {
	m_Value.bValue = inDefaultValue;
	CEngineSettings::get().addCommand(inCommand, this);
	insertToMap([&] {
		ImGui::Checkbox(m_Command, &m_Value.bValue);
	});
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
	insertToMap([&] {
		ImGui::SliderInt(m_Command, &m_Value.iValue.val, m_Value.iValue.min, m_Value.iValue.max);
	});
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
	insertToMap([&] {
		ImGui::SliderFloat(m_Command, &m_Value.fValue.val, m_Value.fValue.min, m_Value.fValue.max);
	});
}
