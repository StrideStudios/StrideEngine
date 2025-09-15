#pragma once

#include <vulkan/vulkan_core.h>
#include <type_traits>

#include "core/Common.h"
#include "core/Registry.h"

class CPass;
class CSceneObject;
class CViewportObject;
class CWorldObject;

#define REGISTER_RENDERER(n) \
	REGISTER_CLASS(n) \
	REGISTER_OBJ(CObjectRendererRegistry, n)

class CObjectRenderer : public SObject {
public:
	virtual void begin() {}

	virtual void render(CPass* inPass, VkCommandBuffer cmd, CSceneObject* inObject, uint32& outDrawCalls, uint64& outVertices) = 0;

	virtual void end() {}
};

DEFINE_REGISTRY(CObjectRenderer)

template <typename TType, typename TPassType>
requires std::is_base_of_v<CViewportObject, TType>
class CViewportObjectRenderer : public CObjectRenderer {
public:

	virtual void render(TPassType* inPass, VkCommandBuffer cmd, TType* inObject, uint32& outDrawCalls, uint64& outVertices) = 0;

	virtual void render(CPass* inPass, VkCommandBuffer cmd, CSceneObject* inObject, uint32& outDrawCalls, uint64& outVertices) override final {
		render(dynamic_cast<TPassType*>(inPass), cmd, dynamic_cast<TType*>(inObject), outDrawCalls, outVertices);
	}
};

template <typename TType, typename TPassType>
requires std::is_base_of_v<CWorldObject, TType>
class CWorldObjectRenderer : public CObjectRenderer {
public:

	virtual void render(TPassType* inPass, VkCommandBuffer cmd, TType* inObject, uint32& outDrawCalls, uint64& outVertices) = 0;

	virtual void render(CPass* inPass, VkCommandBuffer cmd, CSceneObject* inObject, uint32& outDrawCalls, uint64& outVertices) override final {
		render(dynamic_cast<TPassType*>(inPass), cmd, dynamic_cast<TType*>(inObject), outDrawCalls, outVertices);
	}
};
