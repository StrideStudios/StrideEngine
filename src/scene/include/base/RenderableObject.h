#pragma once

#include "SceneObject.h"

class EXPORT CRenderableViewportObject : public CViewportObject {

public:

	virtual SInstancer& getInstancer() {
		return m_Instancer;
	}

	bool isDirty() const {
		return m_IsDirty;
	}

	void setDirty() {
		m_IsDirty = true;
		m_Instancer.setDirty();
	}

private:

	bool m_IsDirty = true;

	SInstancer m_Instancer{1}; //TODO: bad, shouldnt have an instancer in a non-instanced widget
};

class EXPORT CRenderableWorldObject : public CWorldObject {

public:

	virtual SInstancer& getInstancer() {
		return m_Instancer;
	}

	virtual bool isDirty() const {
		return m_IsDirty;
	}

	virtual void setDirty() {
		m_IsDirty = true;
		m_Instancer.setDirty();
	}

	virtual void onMoved() override {
		setDirty();
	}

private:

	bool m_IsDirty = true;

	SInstancer m_Instancer{1}; //TODO: bad, shouldnt have an instancer in a non-instanced widget
};