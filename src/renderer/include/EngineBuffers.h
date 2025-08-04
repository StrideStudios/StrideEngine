#pragma once

#include <span>

#include "Archive.h"
#include "ResourceManager.h"
#include "Swapchain.h"

struct SStaticMesh;

struct NVertex {

};

// Represents a vertex
struct SVertex {
	Vector3f position;
	uint32 uv;
	Vector3f normal;
	uint32 color = 0xffffffff;

	void setUV(const half inUVx, const half inUVy) {
		const uint32 x = inUVx.data & 0xffff;
		const uint32 y = inUVy.data << 16;
		uv = x | y;
	}

	void setColor(const Color4 inColor) {
		color = (inColor.x & 0xff) |
			((inColor.y & 0xff) << 8) |
			((inColor.z & 0xff) << 16) |
			((inColor.w & 0xff) << 24);
	}

	friend CArchive& operator<<(CArchive& inArchive, const SVertex& inVertex) {
		inArchive << inVertex.position;
		inArchive << inVertex.uv;
		inArchive << inVertex.normal;
		inArchive << inVertex.color;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SVertex& inVertex) {
		inArchive >> inVertex.position;
		inArchive >> inVertex.uv;
		inArchive >> inVertex.normal;
		inArchive >> inVertex.color;
		return inArchive;
	}
};

struct SInstance {
	Matrix4f Transform;
};

class CEngineBuffers : public IDestroyable {

	friend class CVulkanRenderer;

public:

	CEngineBuffers();

	~CEngineBuffers();

	SMeshBuffers uploadMesh(CResourceManager& inManager, std::span<uint32> indices, std::span<SVertex> vertices);

};