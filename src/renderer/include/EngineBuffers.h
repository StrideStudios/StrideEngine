#pragma once

#include <span>

#include "Archive.h"
#include "ResourceManager.h"
#include "Swapchain.h"

struct SStaticMesh;

// Represents a vertex
struct SVertex {
	uVector4i posNormUV;
	uint32 color = 0xffffffff;

	void setUV(const half inUVx, const half inUVy) {
		const uint32 x = inUVx.data & 0xffff;
		const uint32 y = inUVy.data << 16;
		posNormUV.w = x | y;
	}

	void setPosAndNorm(const Vector3h inPos, const Vector3h inNorm) {
		Vector2h inPosXY = {inPos.x, inPos.y};
		Vector2h inPosZNormalX = {inPos.z, inNorm.x};
		Vector2h inNormalYZ = {inNorm.y, inNorm.z};
		posNormUV.x = (inPosXY.x.data & 0xffff) | (inPosXY.y.data << 16);
		posNormUV.y = (inPosZNormalX.x.data << 16) | (inPosZNormalX.y.data & 0xffff);
		posNormUV.z = (inNormalYZ.x.data & 0xffff) | (inNormalYZ.y.data << 16);
	}

	void setPosition(const Vector3h inPos) {
		Vector2h inPosXY = {inPos.x, inPos.y};
		posNormUV.x = (inPosXY.x.data & 0xffff) | (inPosXY.y.data << 16);
		posNormUV.y = (inPos.z.data << 16) | (posNormUV.y & 0xffff);
	}

	void setNormal(const Vector3h inNorm) {
		Vector2h inNormalYZ = {inNorm.y, inNorm.z};
		posNormUV.y = (posNormUV.y & 0xffff0000) | (inNorm.x.data & 0xffff);
		posNormUV.z = (inNormalYZ.x.data & 0xffff) | (inNormalYZ.y.data << 16);
	}

	void setColor(const Color4 inColor) {
		color = (inColor.x & 0xff) |
			((inColor.y & 0xff) << 8) |
			((inColor.z & 0xff) << 16) |
			((inColor.w & 0xff) << 24);
	}

	friend CArchive& operator<<(CArchive& inArchive, const SVertex& inVertex) {
		inArchive << inVertex.posNormUV;
		inArchive << inVertex.color;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SVertex& inVertex) {
		inArchive >> inVertex.posNormUV;
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