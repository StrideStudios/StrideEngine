#pragma once

#include "SceneObject.h"

#define MAKE_RENDERABLE \
	public: \
	\
	virtual SInstancer& getInstancer() { \
		return m_Instancer; \
	} \
	virtual const char* getRendererType() = 0; \
	\
	bool isDirty() const { \
		return m_IsDirty; \
	} \
	\
	void setDirty() { \
		m_IsDirty = true; \
		m_Instancer.setDirty(); \
	} \
	\
private: \
	\
	bool m_IsDirty = true; \
	\
	SInstancer m_Instancer{1}; //TODO: bad, shouldnt have an instancer in a non-instanced widget (renderer?)

#define SET_OBJECT_RENDERER(n) \
	public: \
	virtual const char* getRendererType() override { return #n; }

class CRenderableViewportObject : public CViewportObject {
	REGISTER_CLASS(CRenderableViewportObject, CViewportObject)
	MAKE_RENDERABLE
};

class CRenderableWorldObject : public CWorldObject {
	REGISTER_CLASS(CRenderableWorldObject, CWorldObject)
	MAKE_RENDERABLE
};

#undef MAKE_RENDERABLE