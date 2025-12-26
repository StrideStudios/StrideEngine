#pragma once

#include "basic/control/ResourceManager.h"
#include "basic/core/Common.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

typedef struct VkInstance_T* VkInstance;
typedef struct VkDevice_T* VkDevice;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;

class CPass;

/*
 * Base classes for engine access
 */

class CSwapchain : public SObject, public TDirtyable<> {};

class IBuffering {

public:

	void incrementFrame() { m_FrameNumber++; }

	size_t getFrameNumber() const { return m_FrameNumber; };

	size_t getFrameIndex() const { return getFrameNumber() % getFrameOverlap(); }

	virtual size_t getFrameOverlap() const = 0;

private:

	size_t m_FrameNumber = 0;

};

template <typename TType, size_t TFrameOverlap>
class TBuffering : public IBuffering {

public:

	virtual size_t getFrameOverlap() const override { return TFrameOverlap; }

	TType& getFrame(size_t inFrameIndex) { return m_FrameData[inFrameIndex]; }

	const TType& getFrame(size_t inFrameIndex) const { return m_FrameData[inFrameIndex]; }

	TType& getCurrentFrame() { return getFrame(getFrameIndex()); }

	const TType& getCurrentFrame() const { return getFrame(getFrameIndex()); }

	void forEach(const std::function<void(TType&)>& inFunc) {
		for (auto& frame : m_FrameData) {
			inFunc(frame);
		}
	}

private:

	TType m_FrameData[TFrameOverlap];

};

template <typename TType>
class CSingleBuffering final : public TBuffering<TType, 1> {};

template <typename TType>
class CDoubleBuffering final : public TBuffering<TType, 2> {};

struct SRendererInfo {
	TUnique<class CEngineViewport>& viewport;
	TWeak<class CRenderer> renderer;
	TWeak<class CVulkanAllocator> allocator;
};

class CRenderer : public SObject, public IInitializable, public IDestroyable {

public:

	EXPORT static CRenderer* get();

	/*template <typename TType>
	static void create() {
		TType* renderer = new TType();
		set(renderer);
	}*/

	template <typename TPass>
	static void addPass() {
		if (hasPass<TPass>()) return;
		TPass* pass;
		CResourceManager::get().create(pass, get()->getShared().staticCast<CRenderer>());
		get()->m_Passes.push_back(pass);
	}

	template <typename... TPasses>
	static void addPasses() {
		(addPass<TPasses>(), ...);
	}

	template <typename TPass>
	static bool hasPass() {
		return hasPass(TPass::staticClass());
	}

	template <typename TPass>
	static TPass* getPass() {
		return static_cast<TPass*>(getPass(TPass::staticClass()));
	}

	no_discard virtual IBuffering& getBufferingType() = 0;

	no_discard virtual CSwapchain* getSwapchain() = 0;

	virtual void immediateSubmit(std::function<void(struct SCommandBuffer& cmd)>&& function) = 0;

	virtual void render(SRendererInfo& info) = 0;

	virtual bool wait() = 0;

	virtual class CVulkanDevice* device() = 0;

	virtual class CVulkanInstance* instance() = 0;

	std::list<CPass*> getPasses() const { return m_Passes; }

private:

	friend class CEngine;

	EXPORT static CPass* getPass(const SClass* cls);

	EXPORT static bool hasPass(const SClass* cls);

	EXPORT static void set(CRenderer* inRenderer);

	std::list<CPass*> m_Passes;

};