#pragma once

#include <vulkan/vulkan_core.h>
#include <type_traits>

#include "core/Common.h"

class CPass;
struct CPipeline;
class CSceneObject;
class CViewportObject;
class CWorldObject;

#define REGISTER_RENDERER(n) \
	private: \
	struct Class final : SClass { \
		Class() { CClassManager::registerClass(#n, this); }; \
		virtual std::shared_ptr<void> construct() const override { return std::make_shared<n>(); } \
		virtual const char* getName() const override { return #n; } \
	}; \
	inline static Class c{}; \

template <typename TType, typename TPassType>
requires std::is_base_of_v<CSceneObject, TType> and std::is_base_of_v<CPass, TPassType>
class CObjectRenderer {
public:
	virtual ~CObjectRenderer() = default;

	virtual void render(TPassType* inPass, VkCommandBuffer cmd, TType* inObject, uint32& outDrawCalls, uint64& outVertices) = 0;
};

template <typename TType, typename TPassType>
requires std::is_base_of_v<CViewportObject, TType>
class CViewportObjectRenderer : CObjectRenderer<TType, TPassType> {

};

template <typename TType, typename TPassType>
requires std::is_base_of_v<CWorldObject, TType>
class CWorldObjectRenderer : CObjectRenderer<TType, TPassType> {

};
