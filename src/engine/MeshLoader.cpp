#include "MeshLoader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/ext/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <meshoptimizer.h>

#include "Common.h"
#include "Engine.h"
#include "VulkanRenderer.h"
#include "EngineBuffers.h"
#include "GpuScene.h"
#include "fastgltf/core.hpp"
#include "tracy/Tracy.hpp"

struct SVertex;

VkFilter extractFilter(const fastgltf::Filter filter)
{
	switch (filter) {
		// nearest samplers
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;

			// linear samplers
		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapNearest:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_FILTER_LINEAR;
	}
}

VkSamplerMipmapMode extractMipmapMode(const fastgltf::Filter filter)
{
	switch (filter) {
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

void optimizeMesh(std::vector<uint32>& indices, std::vector<SVertex>& vertices) {
	const size_t numIndices = indices.size();
	const size_t numVertices = vertices.size();

	// Create a remap table
	std::vector<uint32> remap(numIndices);
	const size_t optimizedVertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(), numIndices, vertices.data(), numVertices, sizeof(SVertex));

	std::vector<uint32> optimizedIndices;
	std::vector<SVertex> optimizedVertices;
	optimizedIndices.resize(numIndices);
	optimizedVertices.resize(optimizedVertexCount);

	// Remove duplicates
	meshopt_remapIndexBuffer(optimizedIndices.data(), indices.data(), numIndices, remap.data());
	meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), numVertices, sizeof(SVertex), remap.data());

	// Optimize cache
	meshopt_optimizeVertexCache(optimizedIndices.data(), optimizedIndices.data(), numIndices, optimizedVertexCount);

	// reduce overdraw TODO: positions probably wont work
	//meshopt_optimizeOverdraw(optimizedIndices.data(), optimizedIndices.data(), numIndices, &(optimizedVertices[0].position.x), optimizedVertexCount, sizeof(SVertex), 1.05f);

	// optimize access
	meshopt_optimizeVertexFetch(optimizedVertices.data(), optimizedIndices.data(), numIndices, optimizedVertices.data(), optimizedVertexCount, sizeof(SVertex));

	// TODO: simplify mesh?
	/*constexpr float Threshold = 0.5f;
	size_t TargetIndexCount = (size_t)(numIndices * Threshold);
	float TargetError = 0.2f;
	std::vector<uint32> SimplifiedIndices(optimizedIndices.size());
	size_t optimizedIndexCount = meshopt_simplify(SimplifiedIndices.data(), optimizedIndices.data(), numIndices, &optimizedVertices[0].position.x, optimizedVertexCount, sizeof(SVertex), TargetIndexCount, TargetError);

	SimplifiedIndices.resize(optimizedIndexCount);*/

	indices = optimizedIndices;
	vertices = optimizedVertices;
}

//TODO: each glb/gltf mesh will be combined into one with different surfaces, this should lower draw calls to ONLY be the number of surfaces
// and give flexibility to creators of meshes
void CMeshLoader::loadGLTF(CVulkanRenderer* renderer, std::filesystem::path filePath) {
	ZoneScoped;
	std::string string = "Loading: " + filePath.string();
	ZoneName(string.c_str(), string.size());

	msgs("Loading GLTF: {}", filePath.string().c_str());

	std::filesystem::path path = CEngine::get().mAssetPath + filePath.string();

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;

	fastgltf::GltfFileStream data{path};

	fastgltf::Asset gltf;
	fastgltf::Parser parser {};

	auto type = fastgltf::determineGltfFileType(data);
	if (type == fastgltf::GltfType::glTF) {
		if (auto load = parser.loadGltfBinary(data, path.parent_path(), gltfOptions); load) {
			gltf = std::move(load.get());
		} else {
			msgs("Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
			return;
		}
	} else if (type == fastgltf::GltfType::GLB) {
		if (auto load = parser.loadGltfBinary(data, path.parent_path(), gltfOptions); load) {
			gltf = std::move(load.get());
		} else {
			msgs("Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
			return;
		}
	} else {
		msgs("Invalid GLTF file : {}", filePath.string().c_str());
		return;
	}

	// use the same vectors for all meshes so that the memory doesnt reallocate as
	// often
	for (fastgltf::Node& node : gltf.nodes) {
		if (!node.meshIndex.has_value()) continue;
		fastgltf::Mesh& mesh = gltf.meshes[*node.meshIndex];

		auto outMesh = std::make_shared<SStaticMesh>();
		outMesh->name = mesh.name;
		outMesh->transform = Matrix4f(1.f);

		// Add the object
		mLoadedModels.insert(outMesh);

		auto localMatrix = Matrix4f(1.f);

		// Create a local matrix for this mesh so it can be applied to vertices
		{
			std::visit(fastgltf::visitor
				{
					[&](fastgltf::math::fmat4x4 matrix) {
						memcpy(&localMatrix, matrix.data(), sizeof(matrix));
					},
					[&](fastgltf::TRS transform) {
						glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
						glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
						glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

						glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
						glm::mat4 rm = glm::toMat4(rot);
						glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

						localMatrix = tm * rm * sm;
					}
				},
			node.transform);
		}

		std::vector<uint32> indices;
		std::vector<SVertex> vertices;

		uint32 unnamedSurfaceIndex = 0;
		for (auto&& p : mesh.primitives) {
			// primitives without indices should be ignored
			if (gltf.accessors[p.indicesAccessor.value()].count <= 0) continue;

			//TODO: materials defined by + in new material imGui menu, which adds a file and does stuff to it
			auto matData = std::make_shared<SMaterialInstance>();
			matData->passType = EMaterialPass::OPAQUE;
			matData->pipeline = &renderer->mGPUScene->basePass.errorPipeline;

			SStaticMesh::Surface newSurface;
			newSurface.material = matData;
			newSurface.startIndex = (uint32)indices.size();
			newSurface.count = (uint32)gltf.accessors[p.indicesAccessor.value()].count;

			if (p.materialIndex.has_value()) {
				newSurface.name = gltf.materials[p.materialIndex.value()].name;
			} else {
				newSurface.name = fmts("Surface {}", unnamedSurfaceIndex);
				unnamedSurfaceIndex++;
			}

			size_t initial_vtx = vertices.size();

			// load indexes
			{
				fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexaccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
					[&](std::uint32_t idx) {
						indices.push_back(idx + initial_vtx);
					});
			}

			// load vertex positions
			{
				fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
					[&](glm::vec3 v, size_t index) {
						SVertex newvtx;
						newvtx.position = localMatrix * Vector4f{v, 1.f};
						newvtx.normal = { 1, 0, 0 };
						newvtx.color = glm::vec4 { 1.f };
						newvtx.uv_x = 0;
						newvtx.uv_y = 0;
						vertices[initial_vtx + index] = newvtx;
					});
			}

			// load vertex normals
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end()) {

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->accessorIndex],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end()) {

				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->accessorIndex],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].uv_x = v.x;
						vertices[initial_vtx + index].uv_y = v.y;
					});
			}

			// load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end()) {

				// Sometimes the color will not have an alpha, in such a case, the alpha can be set to 1.f
				if (gltf.accessors[colors->accessorIndex].type == fastgltf::AccessorType::Vec3) {
					fastgltf::iterateAccessorWithIndex<Vector3f>(gltf, gltf.accessors[colors->accessorIndex],
						[&](Vector3f v, size_t index) {
							vertices[initial_vtx + index].color = {v, 1.f};
						});
				} else {
					fastgltf::iterateAccessorWithIndex<Vector4f>(gltf, gltf.accessors[colors->accessorIndex],
						[&](Vector4f v, size_t index) {
							vertices[initial_vtx + index].color = v;
						});
				}
			}

			outMesh->surfaces.push_back(newSurface);
		}

		// If no meshes are loaded, do not upload or add to mesh list
		if (indices.empty()) {
			return;
		}

		optimizeMesh(indices, vertices);

		// Calculate Mesh bounds
		{
			// Loop the vertices of this surface, find min/max bounds
			Vector3f minpos = vertices[0].position;
			Vector3f maxpos = vertices[0].position;
			for (int i = 0; i < vertices.size(); i++) {
				minpos = glm::min(minpos, vertices[i].position);
				maxpos = glm::max(maxpos, vertices[i].position);
			}
			// Calculate origin and extents from the min/max, use extent length for radius
			outMesh->bounds.origin = (maxpos + minpos) / 2.f;
			outMesh->bounds.extents = (maxpos - minpos) / 2.f;
			outMesh->bounds.sphereRadius = glm::length(outMesh->bounds.extents);
		}

		outMesh->meshBuffers = renderer->mEngineBuffers->uploadMesh(renderer->mGlobalResourceManager, indices, vertices);

	}

	msgs("GLTF {} Loaded.", filePath.string().c_str());
}