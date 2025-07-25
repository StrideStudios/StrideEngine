#include "MeshLoader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "Common.h"
#include "Engine.h"
#include "VulkanRenderer.h"
#include "EngineBuffers.h"
#include "fastgltf/core.hpp"

struct SVertex;

std::optional<std::vector<std::shared_ptr<SStaticMesh>>> CMeshLoader::loadStaticMesh(CVulkanRenderer* renderer, CEngineBuffers* engineBuffers, std::filesystem::path filePath) {
	msgs("Loading GLTF: {}", filePath.string().c_str());

	std::filesystem::path path = CEngine::get().mAssetPath + filePath.string();

	constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

	fastgltf::GltfFileStream data{path};

	fastgltf::Asset gltf;
	fastgltf::Parser parser {};

	if (auto load = parser.loadGltfBinary(data, path.parent_path(), gltfOptions); load) {
		gltf = std::move(load.get());
	} else {
		msgs("Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
		return {};
	}

	std::vector<std::shared_ptr<SStaticMesh>> meshes;

	std::vector<uint32> indices;
	std::vector<SVertex> vertices;
	for (fastgltf::Mesh& mesh : gltf.meshes) {
		SStaticMesh newMesh;

		newMesh.name = mesh.name;

		indices.clear();
		vertices.clear();

		for (auto&& primitive : mesh.primitives) {
			SStaticMesh::Surface newSurface;
			newSurface.startIndex = (uint32)indices.size();
			newSurface.count = (uint32)gltf.accessors[primitive.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size();

			// Load indexes
			{
				fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
				indices.reserve(indices.size() + indexAccessor.count);

				fastgltf::iterateAccessor<uint32>(gltf, indexAccessor, [&](const uint32 index) {
					indices.push_back(initial_vtx + index);
				});
			}

			// Load vertex positions
			{
				fastgltf::Accessor& posAccessor = gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::fvec3>(gltf, posAccessor, [&](glm::fvec3 vector, size_t index) {
					SVertex vertex;
					vertex.position = {vector.x, vector.y, vector.z};
					vertex.normal = { 1, 0, 0 };
					vertex.color = { 1.f, 1.f, 1.f, 1.f };
					vertex.uv_x = 0;
					vertex.uv_y = 0;
					vertices[initial_vtx + index] = vertex;
				});
			}

			// Load vertex normals
			if (auto normals = primitive.findAttribute("NORMAL"); normals != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::fvec3>(gltf, gltf.accessors[normals->accessorIndex], [&](glm::fvec3 vector, size_t index) {
					vertices[initial_vtx + index].normal = {vector.x, vector.y, vector.z};
				});
			}

			// Load UVs
			if (auto uv = primitive.findAttribute("TEXCOORD_0"); uv != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::fvec2>(gltf, gltf.accessors[uv->accessorIndex], [&](glm::fvec2 vector, size_t index) {
					vertices[initial_vtx + index].uv_x = vector.x;
					vertices[initial_vtx + index].uv_y = vector.y;
				});
			}

			// Load Vertex Colors
			if (auto colors = primitive.findAttribute("COLOR_0"); colors != primitive.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::fvec4>(gltf, gltf.accessors[colors->accessorIndex], [&](glm::fvec4 vector, size_t index) {
					vertices[initial_vtx + index].color = {vector.x, vector.y, vector.z, vector.w};
				});
			}
			newMesh.surfaces.push_back(newSurface);
		}

		// Display the vertex normals
		constexpr bool OverrideColors = true;
		if (OverrideColors) {
			for (SVertex& vertex : vertices) {
				vertex.color = {vertex.normal.x, vertex.normal.y, vertex.normal.z, 1.f};
			}
		}
		newMesh.meshBuffers = engineBuffers->uploadMesh(renderer->mGlobalResourceManager, indices, vertices);

		meshes.emplace_back(std::make_shared<SStaticMesh>(std::move(newMesh)));
	}

	msgs("GLTF {} loaded.", filePath.string().c_str());

	return meshes;
}
