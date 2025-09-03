#pragma once

#include "control/ResourceManager.h"
#include "core/Common.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

class CPass;

/*
 * Base classes for engine access
 */

class CInstance {

public:

	virtual ~CInstance() = default;

	no_discard virtual const vkb::Instance& getInstance() const = 0;

};

class CDevice {

public:

	virtual ~CDevice() = default;

	no_discard virtual const vkb::PhysicalDevice& getPhysicalDevice() const = 0;

	no_discard virtual const vkb::Device& getDevice() const = 0;

};

class CSwapchain {

public:

	virtual ~CSwapchain() = default;

	virtual bool isDirty() const = 0;

	virtual void setDirty() = 0;

};

class CRenderer : public IInitializable<>, public IDestroyable {

public:

	EXPORT static CRenderer* get();

	template <typename TType>
	static void create(const std::list<CPass*>& inPasses) {
		TType* renderer = new TType();
		set(renderer, inPasses);
	}

	static const vkb::Instance& instance() { return get()->getInstance()->getInstance(); }

	static const vkb::PhysicalDevice& physicalDevice() { return get()->getDevice()->getPhysicalDevice(); }

	static const vkb::Device& device() { return get()->getDevice()->getDevice(); }

	no_discard virtual CInstance* getInstance() = 0;

	no_discard virtual CDevice* getDevice() = 0;

	no_discard virtual CSwapchain* getSwapchain() = 0;

	virtual void immediateSubmit(std::function<void(struct SCommandBuffer& cmd)>&& function) = 0;

	virtual void render() = 0;

	virtual bool wait() = 0;

	std::list<CPass*> getPasses() const { return m_Passes; }

	template <typename TType>
	TType* getPass() const {
		for (auto pass : m_Passes) {
			if (TType* typePass = dynamic_cast<TType*>(pass))
				return typePass;
		}
		return nullptr;
	}

protected:

	template <typename TType>
	void addPass() {
		m_Passes.push_back(TType::make());
	}

private:

	EXPORT static void set(CRenderer* inRenderer, const std::list<CPass*>& inPasses);

	std::list<CPass*> m_Passes;

};