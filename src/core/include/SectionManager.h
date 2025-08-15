#pragma once

#include <string>
#include <map>
#include <type_traits>

#include "ResourceManager.h"

#define ADD_SECTION(type, name) \
	public: \
	struct Registry { Registry() { CSectionManager::addSection<type>(name); } }; \
	inline static Registry reg{}; \
	public: \
	static type& get() { return *CSectionManager::getSection<type>(name); } \
	private:

#define CORE_API __declspec(dllexport)

class CSection : public IInitializable<>, public IDestroyable {};

class CRendererSection : public CSection {
public:
	virtual void render() = 0;

	virtual bool wait() = 0;
};

class CSectionManager final {

	std::map<std::string, CSection*> m_Sections;

public:

	CORE_API static CSectionManager& get();

	template <typename TSection>
	requires std::is_base_of_v<CSection, TSection>
	static void addSection(const std::string& inName) {
		msgs("ADD SECTION {}", inName.c_str());
		msgs("Section is {}", get().m_Sections.contains(inName) ? "true" : "false");
		get().m_Sections.insert(std::make_pair(inName, new TSection()));
		msgs("Section is {}", get().m_Sections.contains(inName) ? "true" : "false");
	}

	template <typename TSection>
	requires std::is_base_of_v<CSection, TSection>
	static TSection* getSection(const std::string& inName) {
		if (!get().m_Sections.contains(inName)) {
			errs("Could not construct section {}", inName.c_str());
		}
		return static_cast<TSection*>(get().m_Sections[inName]);
	}
};