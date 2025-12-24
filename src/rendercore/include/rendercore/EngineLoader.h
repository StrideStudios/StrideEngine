#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <memory>

#include "Font.h"
#include "basic/core/Paths.h"
#include "rendercore/StaticMesh.h"

struct SStaticMesh;
class CMaterial;

// Represents a vertex
struct SVertex {
	Vector3f position{0.f};
	uint32 uv = 0u;
	Vector3f normal{0.f};
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

struct SMeshData {

	SMeshData() = default;

	struct Surface {

		std::string name{};

		uint32 startIndex = 0;
		uint32 count = 0;

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

	// We are storing indices as an 'uint24' to reduce storage size
	struct storedIndex {
		uint8 i1, i2, i3;
	};

	bool mHasVertexColor = false;
	std::vector<uint32> indices{};
	std::vector<SVertex> vertices{};
	std::vector<Surface> surfaces{};
	SBounds bounds{};

	friend CArchive& operator<<(CArchive& inArchive, const SMeshData& inData) {
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

class CEngineLoader : public SObject {

	REGISTER_CLASS(CEngineLoader, SObject)
	MAKE_SINGLETON(CEngineLoader)

	template <typename TType>
	static void save(const std::string& inFileName, TType inValue) {
		std::filesystem::path path = SPaths::get()->mAssetPath;
		path.append(inFileName);

		CFileArchive file(path.string(), "wb");
		file << inValue;
		file.close();
	}

	template <typename TType>
	static TType load(const std::filesystem::path& inPath) {
		CFileArchive file(inPath.string(), "rb");

		TType value;
		file >> value;
		file.close();

		return value;
	}

public:

	CEngineLoader() = default;

	EXPORT static void save();

	EXPORT static void load(const TShared<CVulkanAllocator>& allocator);

	//
	// Textures
	//

	EXPORT static void importTexture(const TShared<CVulkanAllocator>& allocator, const std::filesystem::path& inPath);

	static std::map<std::string, SImage_T*>& getImages() { return get()->mImages; }

	std::map<std::string, SImage_T*> mImages{};

	//
	// Fonts
	//

	EXPORT static void importFont(const TShared<CVulkanAllocator>& allocator, const std::filesystem::path& inPath);

	static std::map<std::string, SFont>& getFonts() { return get()->mFonts; }

	std::map<std::string, SFont> mFonts{};

	//
	// Materials
	//

	EXPORT static void createMaterial(const std::string& inMaterialName);

	static std::map<std::string, CMaterial*>& getMaterials() { return get()->mMaterials; }

	std::map<std::string, CMaterial*> mMaterials{};

	//
	// Meshes
	//

	EXPORT static void importMesh(const TShared<CVulkanAllocator>& allocator, const std::filesystem::path& inPath);

	static std::map<std::string, SStaticMesh*>& getMeshes() { return get()->mMeshes; }

	std::map<std::string, SStaticMesh*> mMeshes{};

};
