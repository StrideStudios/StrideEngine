#pragma once

#include "basic/control/ResourceManager.h"
#include "sstl/List.h"

template <typename TType>
struct THierarchy {

	/*
	 * Children
	 */

	virtual void onAddChild(TType* inObject) {}

	virtual void onRemoveChild(TType* inObject) {}

	TList<TUnique<TType>>& getChildren() { return m_Children; }
	const TList<TUnique<TType>>& getChildren() const { return m_Children; }

	template <typename... TArgs>
	void addChild(TArgs&&... args) {
		(m_Children.push(std::forward<TArgs>(args)), ...);
	}

	template <typename... TArgs>
	void removeChild(TArgs&&... args) {
		(m_Children.pop(std::forward<TArgs>(args)), ...);
	}

	TUnique<TType>& operator[](const size_t index) {
		return m_Children[index];
	}

	const TUnique<TType>& operator[](const size_t index) const {
		return m_Children[index];
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
	TList<TUnique<TType>> m_Children;
};
