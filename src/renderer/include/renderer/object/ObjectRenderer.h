#pragma once

#include <vulkan/vulkan_core.h>
#include <type_traits>
#include <map>

#include "core/Common.h"

class CPass;
struct CPipeline;
class CSceneObject;
class CViewportObject;
class CWorldObject;
class CObjectRenderer;

#define REGISTER_RENDERER(n, passType) \
	private: \
	struct Type { \
		Type() { CRendererManager::registerRenderer<n, passType>(#n); }; \
		virtual const char* getName() const { return #n; } \
	}; \
	inline static Type c{};

struct SRendererType {
	virtual ~SRendererType() = default;

	virtual const char* getName() const = 0;
};

class CRendererManager final {

	friend SRendererType;

	std::map<std::string, std::map<std::string, std::shared_ptr<CObjectRenderer>>> m_Renderers{};

public:

	EXPORT static CRendererManager& get();

	template <typename TType, typename TPassType>
	static void registerRenderer(const std::string& inRendererName) {
		registerRenderer(TPassType::name(), inRendererName, std::make_shared<TType>());
	}

	template <typename TPassType>
	requires std::is_base_of_v<CPass, TPassType>
	static void startPass() {
		startPass(TPassType::name());
	}

	template <typename TPassType>
	requires std::is_base_of_v<CPass, TPassType>
	static void endPass() {
		endPass(TPassType::name());
	}

private:

	EXPORT static void registerRenderer(const char* inPassName, const std::string& inRendererName, std::shared_ptr<CObjectRenderer> inRenderer);

	EXPORT static void startPass(const std::string& inPass);

	EXPORT static void endPass(const std::string& inPass);

};

class CObjectRenderer : public SObject {
public:
	virtual void begin() = 0;

	virtual void render(CPass* inPass, VkCommandBuffer cmd, CSceneObject* inObject, uint32& outDrawCalls, uint64& outVertices) = 0;

	virtual void end() = 0;
};

DEFINE_FACTORY(CObjectRenderer)

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
