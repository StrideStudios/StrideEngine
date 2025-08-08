#pragma once

#include <vector>
#include <map>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "imgui.h"

class CGuiText;
class CCommand;

class CEngineSettings {

public:

	constexpr static CEngineSettings& get() {
		static CEngineSettings sEngineSettings;
		return sEngineSettings;
	}

	// Get a certain command, is O(n) but is at compile time
	constexpr static CCommand* getCommand(const std::string& inCommand) {

		CCommand* outCommand = nullptr;
		for (const auto&[command, commandType] : get().m_Commands) {
			if (command == inCommand) {
				outCommand = commandType;
				break;
			}
		}

		return outCommand;
	}

	static void render();

private:

	// Commands need to add themselves
	friend class CCommand;

	// Add command to the settings
	constexpr static void addCommand(const std::string& inCommand, CCommand* inCommandType) {
		//ast(!get().m_Commands.contains(inCommand), "Command {} already defined.", inCommand);
		get().m_Commands.emplace_back(inCommand, inCommandType);
	}

	std::vector<std::pair<std::string, CCommand*>> m_Commands;

};

class CGuiType {

public:

	virtual ~CGuiType() = default;

	constexpr CGuiType(const std::string& inCategory):
	m_Category(inCategory) {}

	no_discard std::string getCategory() const {
		return m_Category;
	}

protected:

	// Only Engine settings needs to access the category map
	friend class CEngineSettings;

	// Each category holds the creation for its individual objects
	// Although it very likely is slow, it only is done on construction
	constexpr static std::vector<std::pair<std::string, std::vector<CGuiType*>>>& getCategoryMap() {
		static std::vector<std::pair<std::string, std::vector<CGuiType*>>> map;
		return map;
	}

	constexpr void insertToMap() {

		for (auto&[command, commands] : getCategoryMap()) {
			if (command == m_Category) {
				commands.insert(
					std::lower_bound(commands.begin(), commands.end(), this, [](const CGuiType* lhs, const CGuiType* rhs) {
						return lhs->getTypeOrder() < rhs->getTypeOrder();
					}),
					this
				);
				return;
			}
		}

		const std::pair<std::string, std::vector<CGuiType*>> item{m_Category, {this}};

		// Order alphabetically
		getCategoryMap().insert(
			std::lower_bound(getCategoryMap().begin(), getCategoryMap().end(), item, [](const std::pair<std::string, std::vector<CGuiType*>>& lhs, const std::pair<std::string, std::vector<CGuiType*>>& rhs) {
				return lhs.first < rhs.first;
			}),
			item
		);

		//getCategoryMap().push_back(item);
	}

	virtual void render() = 0;

	virtual uint32 getTypeOrder() const = 0;

	std::string m_Category;

};

class CGuiText : public CGuiType {

public:

	constexpr CGuiText(const std::string& inCategory)
	: CGuiType(inCategory),
	m_Text("") {
		insertToMap();
	}

	constexpr CGuiText(const std::string& inCategory, const std::string& inText)
	: CGuiType(inCategory),
	m_Text(inText) {
		insertToMap();
	}

	no_discard const std::string& getText() const {
		return m_Text;
	}

	void setText(const std::string& inText) {
		m_Text = inText;
	}

private:

	void render() override {
		ImGui::Text(m_Text.c_str());
	}

	uint32 getTypeOrder() const override {
		return 0;
	}

	std::string m_Text;

};

class CCommand : public CGuiType {

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
	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, bool inDefaultValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::BOOL) {
		m_Value.bValue = inDefaultValue;
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, int32 inDefaultValue, int32 inMinValue, int32 inMaxValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::INT) {
		m_Value.iValue = {
			.val = inDefaultValue,
			.min = inMinValue,
			.max = inMaxValue
		};
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, float inDefaultValue, float inMinValue, float inMaxValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::FLOAT) {
		m_Value.fValue = {
			.val = inDefaultValue,
			.min = inMinValue,
			.max = inMaxValue
		};
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, Extent32 inDefaultValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::EXTENT) {
		m_Value.extent = inDefaultValue;
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, Vector2f inDefaultValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::VECTOR2) {
		m_Value.vector2 = inDefaultValue;
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, Vector3f inDefaultValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::VECTOR3) {
		m_Value.vector3 = inDefaultValue;
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	constexpr CCommand(const std::string& inCategory, const std::string& inCommand, Vector4f inDefaultValue)
	: CGuiType(inCategory),
	m_Command(inCommand),
	m_Type(Type::VECTOR4) {
		m_Value.vector4 = inDefaultValue;
		CEngineSettings::addCommand(inCommand, this);
		insertToMap();
	}

	no_discard const std::string& getCommand() const {
		return m_Command;
	}

protected:

	std::string m_Command;

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

	uint32 getTypeOrder() const override {
		return (uint32)m_Type + 1;
	}
	
};

template <typename TType>
class TCommand final : public CCommand {
	using Type = TType;

public:

	constexpr TCommand(const std::string& inCategory, const std::string& inCommand, Type inDefaultValue)
	: CCommand(inCategory, inCommand, inDefaultValue) {
	}

	constexpr TCommand(const std::string& inCategory, const std::string& inCommand, Type inDefaultValue, Type inMinValue, Type inMaxValue)
	: CCommand(inCategory, inCommand, inDefaultValue, inMinValue, inMaxValue) {
	}

	no_discard Type get() {
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
		errs("Command {} has invalid type.", getCategory());
	}

	no_discard Type getMin() {
		if constexpr(std::is_same_v<Type, int32>) {
			return m_Value.iValue.min;
		} else if constexpr(std::is_same_v<Type, float>) {
			return m_Value.fValue.min;
		}
		errs("Attempted to access min of Command {} that has no min.", getCategory());
	}

	no_discard Type getMax() {
		if constexpr(std::is_same_v<Type, int32>) {
			return m_Value.iValue.max;
		} else if constexpr(std::is_same_v<Type, float>) {
			return m_Value.fValue.max;
		}
		errs("Attempted to access max of Command {} that has no max.", getCategory());
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
		astsNoEntry();
	}

private:

	void render() override {
		if constexpr (std::is_same_v<TType, bool>) {
			ImGui::Checkbox(m_Command.c_str(), &m_Value.bValue);
		} else if constexpr (std::is_same_v<TType, int32>) {
			ImGui::SliderInt(m_Command.c_str(), &m_Value.iValue.val, m_Value.iValue.min, m_Value.iValue.max);
		} else if constexpr (std::is_same_v<TType, float>) {
			ImGui::SliderFloat(m_Command.c_str(), &m_Value.fValue.val, m_Value.fValue.min, m_Value.fValue.max);
		} else if constexpr (std::is_same_v<TType, Extent32>) {
			ImGui::InputInt2(m_Command.c_str(), (int32*)&m_Value.extent);
		} else if constexpr (std::is_same_v<TType, Vector2f>) {
			ImGui::InputFloat2(m_Command.c_str(), (float*)&m_Value.vector2);
		} else if constexpr (std::is_same_v<TType, Vector3f>) {
			ImGui::InputFloat3(m_Command.c_str(), (float*)&m_Value.vector3);
		} else if constexpr (std::is_same_v<TType, Vector4f>) {
			ImGui::InputFloat4(m_Command.c_str(), (float*)&m_Value.vector4);
		}
	}
};

#define ADD_COMMAND(type, command, ...) \
	TCommand<type> command ( \
		SETTINGS_CATEGORY, #command, __VA_ARGS__ \
	)

#define ADD_TEXT(type, ...) \
	CGuiText type(SETTINGS_CATEGORY, __VA_ARGS__)