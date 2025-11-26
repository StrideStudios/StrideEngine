#pragma once

#include "SceneObject.h"

class CObjectRenderer;

struct IRenderable {
	virtual IInstancer& getInstancer() = 0;
};

struct IRenderableClass {
	virtual bool hasRenderer() const = 0;
	virtual CObjectRenderer* getRenderer() const = 0;
	virtual void setRenderer(CObjectRenderer*) = 0;
};

class CRenderableWorldObject;

template <typename TCurrentClass, typename... TParentClasses>
requires std::is_base_of_v<CRenderableWorldObject, TCurrentClass>
struct TClass<TCurrentClass, TParentClasses...> : TGenericClass<TCurrentClass, TParentClasses...>, IRenderableClass {

	using Current = TCurrentClass;

	TClass(const std::string& inName): TGenericClass<TCurrentClass, TParentClasses...>(inName) {
		// Initially set renderer to the same as the parent renderer
		// This can be overridden via specialization
		if (auto parentClass = TClass::getParent()) {
			if (auto renderableParentClass = dynamic_cast<IRenderableClass*>(parentClass)) {
				TClass::setRenderer(renderableParentClass->getRenderer());
			}
		}
	}

	virtual bool hasRenderer() const override { return mRenderer != nullptr; }

	virtual CObjectRenderer* getRenderer() const override { return mRenderer; }

	virtual void setRenderer(CObjectRenderer* inRenderer) override {
		mRenderer = inRenderer;
	}

	CObjectRenderer* mRenderer = nullptr;

};

class CRenderableViewportObject : public CViewportObject, public IRenderable {

	REGISTER_CLASS(CRenderableViewportObject, CViewportObject)

public:

	virtual IInstancer& getInstancer() override {
		return m_Instancer;
	}

private:

	SSingleInstancer m_Instancer;
};

class CRenderableWorldObject : public CWorldObject, public IRenderable {
	REGISTER_CLASS(CRenderableWorldObject, CWorldObject)

public:

	virtual IInstancer& getInstancer() override {
		return m_Instancer;
	}

private:

	SSingleInstancer m_Instancer;
};

#undef MAKE_RENDERABLE