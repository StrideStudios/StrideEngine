#include "MeshLoader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/ext/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Common.h"
#include "Engine.h"
#include "VulkanRenderer.h"
#include "EngineBuffers.h"
#include "EngineTextures.h"
#include "GpuScene.h"
#include "fastgltf/core.hpp"

struct SVertex;

std::optional<std::vector<std::shared_ptr<SStaticMesh>>> CMeshLoader::loadStaticMesh(CVulkanRenderer* renderer, CEngineBuffers* buffers, std::filesystem::path filePath) {
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
		newMesh.meshBuffers = buffers->uploadMesh(renderer->mGlobalResourceManager, indices, vertices);

		meshes.emplace_back(std::make_shared<SStaticMesh>(std::move(newMesh)));
	}

	msgs("GLTF {} loaded.", filePath.string().c_str());

	return meshes;
}

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

std::optional<SImage> loadImage(CVulkanRenderer* renderer, fastgltf::Asset& asset, fastgltf::Image& image) {
    SImage newImage {};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor {
            [](auto& arg) {},
            [&](const fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading
                                                    // local files.

                const std::string path(filePath.uri.path().begin(),
                    filePath.uri.path().end()); // Thanks C++.

                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = renderer->mGlobalResourceManager.allocateImage(data, image.name.c_str(), imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_ASPECT_COLOR_BIT,true);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vector) {
                unsigned char* data = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(vector.bytes.data()), static_cast<int>(vector.bytes.size()),
                    &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = renderer->mGlobalResourceManager.allocateImage(data, image.name.c_str(), imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_ASPECT_COLOR_BIT,true);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];


                std::visit(fastgltf::visitor { // We only care about ArrayWithMime here, because we
                                               // specify LoadExternalBuffers, meaning all buffers
                                               // are already loaded into a Array.
                               [](auto& arg) {},
                               [&](fastgltf::sources::Array& array) {
                                   unsigned char* data = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(array.bytes.data() + bufferView.byteOffset),
                                       static_cast<int>(bufferView.byteLength),
                                       &width, &height, &nrChannels, 4);
                                   if (data) {
                                       VkExtent3D imagesize;
                                       imagesize.width = width;
                                       imagesize.height = height;
                                       imagesize.depth = 1;

                                       newImage = renderer->mGlobalResourceManager.allocateImage(data, image.name.c_str(), imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_ASPECT_COLOR_BIT,true);

                                       stbi_image_free(data);
                                   }
                               } },
                    buffer.data);
            },
        },
        image.data);

    // if any of the attempts to load the data failed, we havent written the image
    // so handle is null
    if (!newImage || newImage->mImage == VK_NULL_HANDLE) {
        return {};
    }
    return newImage;
}

std::optional<std::shared_ptr<SLoadedGLTF>> CMeshLoader::loadGLTF(CVulkanRenderer* renderer, CGPUScene* GPUscene, std::filesystem::path filePath) {
	msgs("Loading GLTF: {}", filePath.string().c_str());

	std::filesystem::path path = CEngine::get().mAssetPath + filePath.string();

	std::shared_ptr<SLoadedGLTF> scene = std::make_shared<SLoadedGLTF>();
	SLoadedGLTF& file = *scene.get();

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
			return {};
		}
	} else if (type == fastgltf::GltfType::GLB) {
		if (auto load = parser.loadGltfBinary(data, path.parent_path(), gltfOptions); load) {
			gltf = std::move(load.get());
		} else {
			msgs("Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
			return {};
		}
	} else {
		msgs("Invalid GLTF file : {}", filePath.string().c_str());
	}

	// we can estimate the descriptors we will need accurately
	std::vector<SDescriptorAllocator::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	file.descriptorPool.init(gltf.materials.size(), sizes);


	// load samplers
	for (fastgltf::Sampler& sampler : gltf.samplers) {

		VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
		samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerCreateInfo.minLod = 0;

		samplerCreateInfo.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		samplerCreateInfo.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		samplerCreateInfo.mipmapMode= extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler = renderer->mGlobalResourceManager.allocateSampler(samplerCreateInfo);

		file.samplers.push_back(newSampler);
	}

	// temporal arrays for all the objects to use while creating the GLTF data
	std::vector<std::shared_ptr<SStaticMesh>> meshes;
	std::vector<std::shared_ptr<SNode>> nodes;
	std::vector<SImage> images;
	std::vector<std::shared_ptr<GLTFMaterial>> materials;

	// load all textures
	for (fastgltf::Image& image : gltf.images) {
		std::optional<SImage> img = loadImage(renderer, gltf, image);

		if (img.has_value()) {
			images.push_back(*img);
			file.images[image.name.c_str()] = *img;
		}
		else {
			// we failed to load, so lets give the slot a default white texture to not
			// completely break loading
			images.push_back(GPUscene->m_ErrorCheckerboardImage);
			std::cout << "gltf failed to load texture " << image.name << std::endl;
		}
	}

	// create buffer to hold the material data
	file.materialDataBuffer = renderer->mGlobalResourceManager.allocateBuffer(sizeof(SGLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
		VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	int dataIndex = 0;
	SGLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (SGLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer->info.pMappedData;
	for (fastgltf::Material& mat : gltf.materials) {
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        SGLTFMetallic_Roughness::MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;

        EMaterialPass passType = EMaterialPass::OPAQUE;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            passType = EMaterialPass::TRANSLUCENT;
        }

        SGLTFMetallic_Roughness::MaterialResources materialResources;
        // default the material textures
        materialResources.colorImage = GPUscene->m_WhiteImage;
        materialResources.colorSampler = GPUscene->m_DefaultSamplerLinear;
        materialResources.metalRoughImage = GPUscene->m_WhiteImage;
        materialResources.metalRoughSampler = GPUscene->m_DefaultSamplerLinear;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer->buffer;
        materialResources.dataBufferOffset = dataIndex * sizeof(SGLTFMetallic_Roughness::MaterialConstants);
        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

        	constants.samplingIDs.x = images[img]->mBindlessAddress;
            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
		// write material parameters to buffer
		sceneMaterialConstants[dataIndex] = constants;
        // build material
        newMat->data = GPUscene->metalRoughMaterial.writeMaterial(passType, materialResources, file.descriptorPool);

        dataIndex++;
    }

	// use the same vectors for all meshes so that the memory doesnt reallocate as
	// often
	std::vector<uint32_t> indices;
	std::vector<SVertex> vertices;

	for (fastgltf::Mesh& mesh : gltf.meshes) {
		std::shared_ptr<SStaticMesh> newmesh = std::make_shared<SStaticMesh>();
		meshes.push_back(newmesh);
		file.meshes[mesh.name.c_str()] = newmesh;
		newmesh->name = mesh.name;

		// clear the mesh arrays each mesh, we dont want to merge them by error
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives) {
			SStaticMesh::Surface newSurface;
			newSurface.startIndex = (uint32_t)indices.size();
			newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

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
						newvtx.position = v;
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

				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[colors->accessorIndex],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
			}

			if (p.materialIndex.has_value()) {
				newSurface.material = materials[p.materialIndex.value()];
			} else {
				newSurface.material = materials[0];
			}

			//loop the vertices of this surface, find min/max bounds
			Vector3f minpos = vertices[initial_vtx].position;
			Vector3f maxpos = vertices[initial_vtx].position;
			for (int i = initial_vtx; i < vertices.size(); i++) {
				minpos = glm::min(minpos, vertices[i].position);
				maxpos = glm::max(maxpos, vertices[i].position);
			}
			// calculate origin and extents from the min/max, use extent lenght for radius
			newSurface.bounds.origin = (maxpos + minpos) / 2.f;
			newSurface.bounds.extents = (maxpos - minpos) / 2.f;
			newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

			newmesh->surfaces.push_back(newSurface);
		}

		newmesh->meshBuffers = renderer->mEngineBuffers->uploadMesh(renderer->mGlobalResourceManager, indices, vertices);
	}

	// load all nodes and their meshes
	for (fastgltf::Node& node : gltf.nodes) {
		std::shared_ptr<SNode> newNode;

		// find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
		if (node.meshIndex.has_value()) {
			newNode = std::make_shared<SMeshNode>();
			static_cast<SMeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
		} else {
			newNode = std::make_shared<SNode>();
		}

		nodes.push_back(newNode);
		file.nodes[node.name.c_str()];

		std::visit(fastgltf::visitor { [&](fastgltf::math::fmat4x4 matrix) {
										  memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
									  },
					   [&](fastgltf::TRS transform) {
						   glm::vec3 tl(transform.translation[0], transform.translation[1],
							   transform.translation[2]);
						   glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
							   transform.rotation[2]);
						   glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

						   glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
						   glm::mat4 rm = glm::toMat4(rot);
						   glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

						   newNode->localTransform = tm * rm * sm;
					   } },
			node.transform);
	}

	// run loop again to setup transform hierarchy
	for (int i = 0; i < gltf.nodes.size(); i++) {
		fastgltf::Node& node = gltf.nodes[i];
		std::shared_ptr<SNode>& sceneNode = nodes[i];

		for (auto& c : node.children) {
			sceneNode->children.push_back(nodes[c]);
			nodes[c]->parent = sceneNode;
		}
	}

	// find the top nodes, with no parents
	for (auto& node : nodes) {
		if (node->parent.lock() == nullptr) {
			file.topNodes.push_back(node);
			node->refreshTransform(glm::mat4 { 1.f });
		}
	}
	return scene;
}