#pragma once

#include <filesystem>
#include <set>

#include "Archive.h"
#include "EngineBuffers.h"
#include "ResourceManager.h"
#include "StaticMesh.h"

struct SMeshData {

	SMeshData() = default;

	struct Surface {

		std::string name;

		uint32 startIndex;
		uint32 count;

		friend CArchive& operator<<(CArchive& inArchive, const Surface& inSurface) {
			inArchive << inSurface.name;
			inArchive << inSurface.startIndex;
			inArchive << inSurface.count;
			return inArchive;
		}

		friend CArchive& operator>>(CArchive& inArchive, Surface& inSurface) {
			inArchive >> inSurface.name;
			inArchive >> inSurface.startIndex;
			inArchive >> inSurface.count;
			return inArchive;
		}
	};

	std::string name;

	// We are storing indices as an 'uint24' to reduce storage size
	struct storedIndex {
		uint8 i1, i2, i3;
	};

	bool mHasVertexColor;
	std::vector<uint32> indices;
	std::vector<SVertex> vertices;
	std::vector<Surface> surfaces;
	SBounds bounds;

	friend CArchive& operator<<(CArchive& inArchive, const SMeshData& inData) {
		inArchive << inData.name;
		inArchive << inData.mHasVertexColor;

		std::vector<uint8> indices0;
		std::vector<uint16> indices1;
		std::vector<storedIndex> indices2;
		std::vector<uint32> indices3;

		// Smart index pushing to remove spaces inbetween indices
		for (auto index : inData.indices) {
			if ((index & 0xff00) == 0 && indices1.empty()) {
				indices0.push_back(index & 0xff);
				continue;
			}
			if ((index & 0xff0000) == 0 && indices2.empty()) {
				indices1.push_back(index & 0xffff);
				continue;
			}
			if ((index & 0xff000000) == 0 && indices3.empty()) {
				uint8 i1 = index & 0xff;
				uint8 i2 = (index & 0xff00) >> 8;
				uint8 i3 = (index & 0xff0000) >> 16;
				indices2.push_back({i1, i2, i3});
				continue;
			}
			indices3.push_back(index);
		}

		inArchive << indices0;
		inArchive << indices1;
		inArchive << indices2;
		inArchive << indices3;
		inArchive << inData.vertices.size();

		// Since vertex colors are optional, they are not always serialized
		for (const auto& value : inData.vertices) {
			inArchive << value.position;
			inArchive << value.uv;
			inArchive << value.normal;
			if (inData.mHasVertexColor)
				inArchive << value.color;
		}

		inArchive << inData.surfaces;
		inArchive << inData.bounds;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SMeshData& inData) {
		inArchive >> inData.name;
		inArchive >> inData.mHasVertexColor;

		std::vector<uint8> indices0;
		std::vector<uint16> indices1;
		std::vector<storedIndex> indices2;
		std::vector<uint32> indices3;

		inArchive >> indices0;
		inArchive >> indices1;
		inArchive >> indices2;
		inArchive >> indices3;

		inData.indices.append_range(indices0);
		inData.indices.append_range(indices1);
		for (auto [i1, i2, i3] : indices2) {
			const int32 i = i1 | i2 << 8 | i3 << 16;
			inData.indices.push_back(i);
		}
		inData.indices.append_range(indices3);

		size_t size;
		inArchive >> size;
		inData.vertices.resize(size);
		for (size_t i = 0; i < size; ++i) {
			inArchive >> inData.vertices[i].position;
			inArchive >> inData.vertices[i].uv;
			inArchive >> inData.vertices[i].normal;
			if (inData.mHasVertexColor) {
				inArchive >> inData.vertices[i].color;
			}
		}

		inArchive >> inData.surfaces;
		inArchive >> inData.bounds;
		return inArchive;
	}
};

// TODO: automatically load meshes, and have some members for that
class CMeshLoader : public IDestroyable {
public:

	CMeshLoader() = default;

	void loadGLTF(CVulkanRenderer* renderer, std::filesystem::path filePath);

	void loadGLTFExternal(CVulkanRenderer* renderer, std::filesystem::path filePath);

	std::vector<std::shared_ptr<SStaticMesh>> mLoadedModels{};

private:

	void loadGLTF_Internal(CVulkanRenderer* renderer, std::filesystem::path path);
};
