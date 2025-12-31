#pragma once

#include "basic/core/Common.h"
#include "sstl/Array.h"

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
	virtual ~IBuffering() = default;

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

	TBuffering() = default;

	TArray<TUnique<TType>, TFrameOverlap>& data() { return m_FrameData; }

	virtual size_t getFrameOverlap() const override { return TFrameOverlap; }

	TType& getFrame(size_t inFrameIndex) { return *m_FrameData[inFrameIndex].get(); }

	const TType& getFrame(size_t inFrameIndex) const { return *m_FrameData[inFrameIndex].get(); }

	TType& getCurrentFrame() { return getFrame(getFrameIndex()); }

	const TType& getCurrentFrame() const { return getFrame(getFrameIndex()); }

private:

	TArray<TUnique<TType>, TFrameOverlap> m_FrameData;

};

template <typename TType>
using CSingleBuffering = TBuffering<TType, 1>;

template <typename TType>
using CDoubleBuffering = TBuffering<TType, 2>;

struct SRendererInfo {
	TFrail<class CScene> scene = nullptr;
	TFrail<class CEngineViewport> viewport = nullptr;
	TFrail<class CRenderer> renderer = nullptr;
	TFrail<class CVulkanAllocator> allocator = nullptr;
};

class CRenderer : public SObject, public IInitializable, public IDestroyable {

public:

	/*template <typename TType>
	static void create() {
		TType* renderer = new TType();
		set(renderer);
	}*/

	template <typename TPass>
	void addPass() {
		if (hasPass<TPass>()) return;
		m_Passes.push_back(TUnique<TPass>{this});
	}

	template <typename... TPasses>
	void addPasses() {
		(addPass<TPasses>(), ...);
	}

	template <typename TPass>
	bool hasPass() {
		return hasPass(TPass::staticClass());
	}

	template <typename TPass>
	TPass* getPass() {
		return static_cast<TPass*>(getPass(TPass::staticClass()));
	}

	no_discard virtual IBuffering& getBufferingType() = 0;

	no_discard virtual TFrail<CSwapchain> getSwapchain() = 0;

	virtual void immediateSubmit(std::function<void(struct SCommandBuffer& cmd)>&& function) = 0;

	virtual void render(SRendererInfo& info) = 0;

	virtual bool wait() = 0;

	virtual TFrail<class CVulkanDevice> device() = 0;

	virtual TFrail<class CVulkanInstance> instance() = 0;

	std::list<TUnique<CPass>>& getPasses() { return m_Passes; }

	const std::list<TUnique<CPass>>& getPasses() const { return m_Passes; }

	EXPORT virtual void destroy() override;

private:

	friend class CEngine;

	EXPORT CPass* getPass(const SClass* cls) const;

	EXPORT bool hasPass(const SClass* cls) const;

	std::list<TUnique<CPass>> m_Passes;

};