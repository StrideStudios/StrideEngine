#pragma once

#include "SceneObject.h"

class CObjectRenderer;

#define MAKE_RENDERABLE \
	public: \
	\
	virtual SInstancer& getInstancer() { \
		return m_Instancer; \
	} \
	virtual const char* getRendererType() { return ""; }; \
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

struct IRenderableClass {
	virtual CObjectRenderer* getRenderer() const = 0;
};

class CRenderableWorldObject;

template <typename TCurrentClass, typename... TParentClasses>
requires std::is_base_of_v<CRenderableWorldObject, TCurrentClass>
struct TClass<TCurrentClass, TParentClasses...> : TGenericClass<TCurrentClass, TParentClasses...>, IRenderableClass {

	using Current = TCurrentClass;

	TClass(const std::string& inName): TGenericClass<TCurrentClass, TParentClasses...>(inName) {}

	virtual CObjectRenderer* getRenderer() const override {
		return nullptr;
	}

};

class CRenderableViewportObject : public CViewportObject {
	REGISTER_CLASS(CRenderableViewportObject, CViewportObject)
	MAKE_RENDERABLE
};

class CRenderableWorldObject : public CWorldObject {
	REGISTER_CLASS(CRenderableWorldObject, CWorldObject)
	MAKE_RENDERABLE
};

#undef MAKE_RENDERABLE