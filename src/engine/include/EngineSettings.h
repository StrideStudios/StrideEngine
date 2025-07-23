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
		FLOAT,
		EXTENT,
		VECTOR2,
		VECTOR3,
		VECTOR4
	};

public:

	CCommand(const char* inCategory, const char* inCommand, bool inDefaultValue);

	CCommand(const char* inCategory, const char* inCommand, int32 inDefaultValue, int32 inMinValue, int32 inMaxValue);

	CCommand(const char* inCategory, const char* inCommand, float inDefaultValue, float inMinValue, float inMaxValue);

	CCommand(const char* inCategory, const char* inCommand, Extent32 inDefaultValue);

	CCommand(const char* inCategory, const char* inCommand, Vector2f inDefaultValue);

	CCommand(const char* inCategory, const char* inCommand, Vector3f inDefaultValue);

	CCommand(const char* inCategory, const char* inCommand, Vector4f inDefaultValue);

	std::string getCategory() const {
		return m_Category;
	}

	bool getBool() {
		ast(m_Type == Type::BOOL, "Tried to access Condition {} as bool.", getCategory());
		return m_Value.bValue;
	}

	int32 getInt() {
		ast(m_Type == Type::INT, "Tried to access Condition {} as int.", getCategory());
		return m_Value.iValue.val;
	}

	float getFloat() {
		ast(m_Type == Type::FLOAT, "Tried to access Condition {} as float.", getCategory());
		return m_Value.fValue.val;
	}

	Extent32 getExtent() {
		ast(m_Type == Type::EXTENT, "Tried to access Condition {} as extent.", getCategory());
		return m_Value.extent;
	}

	Vector2f getVector2f() {
		ast(m_Type == Type::VECTOR2, "Tried to access Condition {} as vector2f.", getCategory());
		return m_Value.vector2;
	}

	Vector3f getVector3f() {
		ast(m_Type == Type::VECTOR3, "Tried to access Condition {} as vector3f.", getCategory());
		return m_Value.vector3;
	}

	Vector4f getVector4f() {
		ast(m_Type == Type::VECTOR4, "Tried to access Condition {} as vector4f.", getCategory());
		return m_Value.vector4;
	}

	void setBool(bool inValue) {
		ast(m_Type == Type::BOOL, "Tried to set Condition {} as bool.", getCategory());
		m_Value.bValue = inValue;
	}

	void setInt(int32 inValue) {
		ast(m_Type == Type::INT, "Tried to set Condition {} as int.", getCategory());
		m_Value.iValue.val = inValue;
	}

	void setFloat(float inValue) {
		ast(m_Type == Type::FLOAT, "Tried to set Condition {} as float.", getCategory());
		m_Value.fValue.val = inValue;
	}

	void setExtent(Extent32 inValue) {
		ast(m_Type == Type::EXTENT, "Tried to set Condition {} as extent.", getCategory());
		m_Value.extent = inValue;
	}

	void setVector2f(Vector2f inValue) {
		ast(m_Type == Type::VECTOR2, "Tried to set Condition {} as vector2f.", getCategory());
		m_Value.vector2 = inValue;
	}

	void setVector3f(Vector3f inValue) {
		ast(m_Type == Type::VECTOR3, "Tried to set Condition {} as vector3f.", getCategory());
		m_Value.vector3 = inValue;
	}

	void setVector4f(Vector4f inValue) {
		ast(m_Type == Type::VECTOR4, "Tried to set Condition {} as vector4f.", getCategory());
		m_Value.vector4 = inValue;
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
		Extent32 extent;
		Vector2f vector2;
		Vector3f vector3;
		Vector4f vector4;
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
		ast(!get().m_Commands.contains(inCommand), "Command {} already defined.", inCommand);
		get().m_Commands.insert({inCommand, inCommandType});
	}

	std::map<std::string, CCommand*> m_Commands;

};