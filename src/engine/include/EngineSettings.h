#pragma once

#include <functional>
#include <map>
#include <vector>

#include "BasicTypes.h"
#include "Common.h"

class CCommand {

	enum class Type : uint8 {
		BOOL,
		INT,
		FLOAT
	};

public:

	CCommand(const char* inCategory, const char* inCommand, bool inDefaultValue);

	CCommand(const char* inCategory, const char* inCommand, int32 inDefaultValue, int32 inMinValue, int32 inMaxValue);

	CCommand(const char* inCategory, const char* inCommand, float inDefaultValue, float inMinValue, float inMaxValue);

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

private:

	// Only Engine settings needs to access the category map
	friend class CEngineSettings;

	// Each category holds the creation for its individual objects
	// Although it very likely is slow, it only is done on initialization
	static std::map<std::string, std::vector<CCommand*>>& getCategoryMap() {
		static std::map<std::string, std::vector<CCommand*>> map;
		return map;
	}

	void insertToMap() {
		if (getCategoryMap().contains(m_Category)) {
			getCategoryMap()[m_Category].push_back(this);
		} else {
			getCategoryMap().insert({m_Category, {this}});
		}
	}

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

	// Get a certain command
	static CCommand* getCommand(const char* inCommand) {
		if (!get().m_Commands.contains(inCommand)) {
			msg("Attempted to access command {} that did not exist!", inCommand);
			return nullptr;
		}
		return get().m_Commands.at(inCommand);
	}

	// Render the commands on ImGui
	static void render();

private:

	// Commands need to add themselves
	friend class CCommand;

	// Add command to the settings
	static void addCommand(const char* inCommand, CCommand* inCommandType) {
		assert(!get().m_Commands.contains(inCommand));
		get().m_Commands.insert({inCommand, inCommandType});
	}

	std::map<std::string, CCommand*> m_Commands;

};