#pragma once

#include "basic/control/ResourceManager.h"

template <typename TType>
struct THierarchy {

	using Storage = CResourceManager;

	/*
	 * Children
	 */

	virtual void onAddChild(TType* inObject) {}

	virtual void onRemoveChild(TType* inObject) {}

	const Storage::Storage& getChildren() const {
		return m_Children.getObjects();
	}

	template <typename TChildType>
	requires std::is_base_of_v<TType, TChildType>
	void addChild(TChildType*& inObject) {
		inObject->itr = m_Children.push(inObject);
		onAddChild(inObject);
	}

	template <typename TChildType, typename... TArgs>
	requires std::is_base_of_v<TType, TChildType>
	void createChild(TChildType*& inObject, TArgs... args) {
		inObject->itr = m_Children.create(inObject, args...);
		onAddChild(inObject);
	}

	template <typename TChildType>
	requires std::is_base_of_v<TType, TChildType>
	void removeChild(TChildType*& inObject) {
		m_Children.remove(inObject->itr);
		onRemoveChild(inObject);
	}

	TType* operator[](const size_t index) {
		return dynamic_cast<TType*>(m_Children[index]);
	}

	const TType* operator[](const size_t index) const {
		return dynamic_cast<TType*>(m_Children[index]);
	}

private:

	friend CArchive& operator<<(CArchive& inArchive, const THierarchy& inHierarchy) {
		inArchive << inHierarchy.m_Children;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, THierarchy& inHierarchy) {
		inArchive >> inHierarchy.m_Children;
		return inArchive;
	}

	/*
	 * A Resource Manager that contains Object children
	 */
	Storage m_Children;
};