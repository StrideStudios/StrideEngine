#pragma once

#include <functional>
#include <map>
#include <vector>

#include "BasicTypes.h"
#include "Common.h"
#include "imgui.h"
#include "imgui_internal.h"

class CCommand {

	enum class Type : uint8 {
		BOOL,
		INT,
		FLOAT
	};

	// Each category holds the creation for its individual objects
	// Will probably be slow, however
	static std::map<std::string, std::vector<std::function<void()>>>& getCategoryMap() {
		static std::map<std::string, std::vector<std::function<void()>>> map;
		return map;
	}

	void insertToMap(std::function<void()> func) {
		if (getCategoryMap().contains(m_Category)) {
			getCategoryMap()[m_Category].push_back(func);
		} else {
			getCategoryMap().insert({m_Category, {func}});
		}
	}

public:

	CCommand(const char* inCategory, const char* inCommand, bool inDefaultValue);

	CCommand(const char* inCategory, const char* inCommand, int32 inDefaultValue, int32 inMinValue, int32 inMaxValue);

	CCommand(const char* inCategory, const char* inCommand, float inDefaultValue, float inMinValue, float inMaxValue);

	static void render() {
		for (auto&[category, commands] : getCategoryMap()) {
			if (ImGui::BeginTabItem(category.c_str())) {
				for (const auto& command : commands) {
					command();
				}
				ImGui::EndTabItem();
			}
		}
	}

	std::string getCategory() const {
		return m_Category;
	}

	bool getBool() {
		assert(m_Type == Type::BOOL);
		return m_Value.bValue;
	}

	int32 getInt() {
		assert(m_Type == Type::INT);
		return m_Value.iValue.val;
	}

	float getFloat() {
		assert(m_Type == Type::FLOAT);
		return m_Value.fValue.val;
	}

	void setBool(bool inValue) {
		assert(m_Type == Type::BOOL);
		m_Value.bValue = inValue;
	}

	void setInt(int32 inValue) {
		assert(m_Type == Type::INT);
		m_Value.iValue.val = inValue;
	}

	void setFloat(float inValue) {
		assert(m_Type == Type::FLOAT);
		m_Value.fValue.val = inValue;
	}

public:

	const char* m_Category;

	const char* m_Command;

	Type m_Type;

	union {
		bool bValue;
		struct {
			int32 val;
			int32 min;
			int32 max;
		} iValue;
		struct {
			float val;
			float min;
			float max;
		} fValue;
	} m_Value;
};

#define ADD_COMMAND(command, ...) \
	CCommand command ( \
		COMMAND_CATEGORY, #command, __VA_ARGS__ \
	)

class CEngineSettings {

public:

	static CEngineSettings& get() {
		static CEngineSettings sEngineSettings;
		return sEngineSettings;
	}

	// Get a certain commands value
	static CCommand* getCommand(const char* inCommand) {
		if (!get().m_Commands.contains(inCommand)) {
			msg("Attempted to access command {} that did not exist!", inCommand);
			return nullptr;
		}
		return get().m_Commands.at(inCommand);
	}

	// Set a certain commands value
	static void setCommand(const char* inCommand, float inValue) {
		//get().m_Commands[inCommand].set(inValue);
	}

	// Render the commands on ImGui
	static void render() {
		if (ImGui::Begin("Engine Settings")) {
			if (ImGui::BeginTabBar("Settings")) {
				CCommand::render();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}

	// Add command to the settings
	static void addCommand(const char* inCommand, CCommand* inCommandType) {
		assert(!get().m_Commands.contains(inCommand));
		get().m_Commands.insert({inCommand, inCommandType});
	}

private:

	std::map<std::string, CCommand*> m_Commands;

};