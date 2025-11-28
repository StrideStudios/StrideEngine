#pragma once

#include <vulkan/vulkan_core.h>
#include <type_traits>

#include "basic/core/Common.h"
#include "basic/core/Registry.h"
#include "rendercore/RenderStack.h"

class CPass;
class CSceneObject;
class CViewportObject;
class CWorldObject;

#define REGISTER_RENDERER(n, object, ...) \
	REGISTER_CLASS(n, __VA_ARGS__) \
	REGISTER_OBJ(CObjectRendererRegistry, n) \
	STATIC_C_BLOCK( \
		object::staticClass()->setRenderer(CObjectRendererRegistry::get(#n)); \
	)

class CObjectRenderer : public SObject {

	REGISTER_CLASS(CObjectRenderer, SObject)

public:
	virtual void begin() {}

	virtual void render(CPass* inPass, VkCommandBuffer cmd, const SRenderStack& stack, CSceneObject* inObject, size_t& outDrawCalls, size_t& outVertices) = 0;

	virtual void end() {}
};

DEFINE_REGISTRY(CObjectRenderer)

template <typename TType, typename TPassType>
requires std::is_base_of_v<CViewportObject, TType>
class CViewportObjectRenderer : public CObjectRenderer {

	REGISTER_CLASS(CViewportObjectRenderer, CObjectRenderer)

public:

	virtual void render(TPassType* inPass, VkCommandBuffer cmd, const SRenderStack2f& stack, TType* inObject, size_t& outDrawCalls, size_t& outVertices) = 0;

	virtual void render(CPass* inPass, VkCommandBuffer cmd, const SRenderStack& stack, CSceneObject* inObject, size_t& outDrawCalls, size_t& outVertices) override final {
		render(dynamic_cast<TPassType*>(inPass), cmd, static_cast<SRenderStack2f>(stack), dynamic_cast<TType*>(inObject), outDrawCalls, outVertices);
	}
};

template <typename TType, typename TPassType>
requires std::is_base_of_v<CWorldObject, TType>
class CWorldObjectRenderer : public CObjectRenderer {

	REGISTER_CLASS(CWorldObjectRenderer, CObjectRenderer)

public:

	virtual void render(TPassType* inPass, VkCommandBuffer cmd, const SRenderStack3f& stack, TType* inObject, size_t& outDrawCalls, size_t& outVertices) = 0;

	virtual void render(CPass* inPass, VkCommandBuffer cmd, const SRenderStack& stack, CSceneObject* inObject, size_t& outDrawCalls, size_t& outVertices) override final {
		render(dynamic_cast<TPassType*>(inPass), cmd, static_cast<SRenderStack3f>(stack), dynamic_cast<TType*>(inObject), outDrawCalls, outVertices);
	}
};
