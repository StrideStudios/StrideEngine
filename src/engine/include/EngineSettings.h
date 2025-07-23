#pragma once

#include <vector>
#include <map>
#include "Common.h"
#include "imgui.h"

class CCommand;

class CEngineSettings {

public:

	constexpr static CEngineSettings& get() {
		static CEngineSettings sEngineSettings;
		return sEngineSettings;
	}

	// Get a certain command, is O(n) but is at compile time
	constexpr static CCommand* getCommand(const char* inCommand) {

		CCommand* outCommand = nullptr;
		for (const auto&[command, commandType] : get().m_Commands) {
			if (command == inCommand) {
				outCommand = commandType;
				break;
			}
		}

		return outCommand;
	}

	// Render the commands on ImGui
	static void render();

private:

	// Commands need to add themselves
	friend class CCommand;

	// Add command to the settings
	constexpr static void addCommand(const char* inCommand, CCommand* inCommandType) {
		//ast(!get().m_Commands.contains(inCommand), "Command {} already defined.", inCommand);
		get().m_Commands.push_back({inCommand, inCommandType});
	}

	std::vector<std::pair<std::string, CCommand*>> m_Commands;

};

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
	virtual ~CCommand() = default;

	constexpr CCommand(const char* inCategory, const char* inCommand, bool inDefaultValue)
	: m_Category(inCategory),
	m_Command(inCommand),
	m_Type(Type::BOOL) {
		m_Value.bValue = inDefaultValue;
		CEngineSettings::get().addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const char* inCategory, const char* inCommand, int32 inDefaultValue, int32 inMinValue, int32 inMaxValue)
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

	constexpr CCommand(const char* inCategory, const char* inCommand, float inDefaultValue, float inMinValue, float inMaxValue)
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

	constexpr CCommand(const char* inCategory, const char* inCommand, Extent32 inDefaultValue)
	: m_Category(inCategory),
	m_Command(inCommand),
	m_Type(Type::EXTENT) {
		m_Value.extent = inDefaultValue;
		CEngineSettings::get().addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const char* inCategory, const char* inCommand, Vector2f inDefaultValue)
	: m_Category(inCategory),
	m_Command(inCommand),
	m_Type(Type::VECTOR2) {
		m_Value.vector2 = inDefaultValue;
		CEngineSettings::get().addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const char* inCategory, const char* inCommand, Vector3f inDefaultValue)
	: m_Category(inCategory),
	m_Command(inCommand),
	m_Type(Type::VECTOR3) {
		m_Value.vector3 = inDefaultValue;
		CEngineSettings::get().addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const char* inCategory, const char* inCommand, Vector4f inDefaultValue)
	: m_Category(inCategory),
	m_Command(inCommand),
	m_Type(Type::VECTOR4) {
		m_Value.vector4 = inDefaultValue;
		CEngineSettings::get().addCommand(inCommand, this);
		insertToMap();
	}

	std::string getCategory() const {
		return m_Category;
	}

	std::string getCommand() const {
		return m_Command;
	}

protected:

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

private:

	// Only Engine settings needs to access the category map
	friend class CEngineSettings;

	// Each category holds the creation for its individual objects
	// Although it very likely is slow, it only is done on construction
	constexpr static std::vector<std::pair<std::string, std::vector<CCommand*>>>& getCategoryMap() {
		static std::vector<std::pair<std::string, std::vector<CCommand*>>> map;
		return map;
	}

	constexpr void insertToMap() {

		for (auto&[command, commands] : getCategoryMap()) {
			if (command == m_Category) {
				commands.push_back(this);
				return;
			}
		}

		getCategoryMap().push_back({m_Category, {this}});
	}

	virtual void render() = 0;
};

template <typename TType>
class TCommand final : public CCommand {
	using Type = TType;

public:

	constexpr TCommand(const char* inCategory, const char* inCommand, Type inDefaultValue)
	: CCommand(inCategory, inCommand, inDefaultValue) {
	}

	constexpr TCommand(const char* inCategory, const char* inCommand, Type inDefaultValue, Type inMinValue, Type inMaxValue)
	: CCommand(inCategory, inCommand, inDefaultValue, inMinValue, inMaxValue) {
	}

	Type get() {
		if constexpr(std::is_same_v<Type, bool>) {
			return m_Value.bValue;
		} else if constexpr(std::is_same_v<Type, int32>) {
			return m_Value.iValue.val;
		} else if constexpr(std::is_same_v<Type, float>) {
			return m_Value.fValue.val;
		} else if constexpr(std::is_same_v<Type, Extent32>) {
			return m_Value.extent;
		} else if constexpr(std::is_same_v<Type, Vector2f>) {
			return m_Value.vector2;
		} else if constexpr(std::is_same_v<Type, Vector3f>) {
			return m_Value.vector3;
		} else if constexpr(std::is_same_v<Type, Vector4f>) {
			return m_Value.vector4;
		}
		err("Command {} has invalid type.", m_Category);
	}

	Type getMin() {
		if constexpr(std::is_same_v<Type, int32>) {
			return m_Value.iValue.min;
		} else if constexpr(std::is_same_v<Type, float>) {
			return m_Value.fValue.min;
		}
		err("Attempted to access min of Command {} that has no min.", m_Category);
	}

	Type getMax() {
		if constexpr(std::is_same_v<Type, int32>) {
			return m_Value.iValue.max;
		} else if constexpr(std::is_same_v<Type, float>) {
			return m_Value.fValue.max;
		}
		err("Attempted to access max of Command {} that has no max.", m_Category);
	}

	void set(Type inValue) {
		if constexpr(std::is_same_v<Type, bool>) {
			m_Value.bValue = inValue;
			return;
		} else if constexpr(std::is_same_v<Type, int32>) {
			m_Value.iValue.val = inValue;
			return;
		} else if constexpr(std::is_same_v<Type, float>) {
			m_Value.fValue.val = inValue;
			return;
		} else if constexpr(std::is_same_v<Type, Extent32>) {
			m_Value.extent = inValue;
			return;
		} else if constexpr(std::is_same_v<Type, Vector2f>) {
			m_Value.vector2 = inValue;
			return;
		} else if constexpr(std::is_same_v<Type, Vector3f>) {
			m_Value.vector3 = inValue;
			return;
		} else if constexpr(std::is_same_v<Type, Vector4f>) {
			m_Value.vector4 = inValue;
			return;
		}
		astNoEntry();
	}

private:

	void render() override {
		if constexpr (std::is_same_v<TType, bool>) {
			ImGui::Checkbox(m_Command, &m_Value.bValue);
		} else if constexpr (std::is_same_v<TType, int32>) {
			ImGui::SliderInt(m_Command, &m_Value.iValue.val, m_Value.iValue.min, m_Value.iValue.max);
		} else if constexpr (std::is_same_v<TType, float>) {
			ImGui::SliderFloat(m_Command, &m_Value.fValue.val, m_Value.fValue.min, m_Value.fValue.max);
		} else if constexpr (std::is_same_v<TType, Extent32>) {
			ImGui::InputInt2(m_Command, (int32*)&m_Value.extent);
		} else if constexpr (std::is_same_v<TType, Vector2f>) {
			ImGui::InputFloat2(m_Command, (float*)&m_Value.vector2);
		} else if constexpr (std::is_same_v<TType, Vector3f>) {
			ImGui::InputFloat3(m_Command, (float*)&m_Value.vector3);
		} else if constexpr (std::is_same_v<TType, Vector4f>) {
			ImGui::InputFloat4(m_Command, (float*)&m_Value.vector4);
		}
	}
};

#define ADD_COMMAND(type, command, ...) \
	TCommand<type> command ( \
		COMMAND_CATEGORY, #command, __VA_ARGS__ \
	)
