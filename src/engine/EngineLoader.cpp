#include "EngineLoader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/core.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <tracy/Tracy.hpp>

#include <glm/gtx/quaternion.hpp>

#include <meshoptimizer.h>

#include "Engine.h"
#include "EngineTextures.h"
#include "encoder/basisu_comp.h"
#include "transcoder/basisu_transcoder.h"
#include "encoder/basisu_gpu_texture.h"

#include "StaticMesh.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

constexpr static bool gUseOpenCL = false;

SImage_T* loadImage(CVulkanResourceManager& allocator, const std::filesystem::path& path) {
	const std::string& fileName = path.filename().string();

	basisu::uint8_vec fileData;
	basisu::read_file_to_vec(path.string().c_str(), fileData);

	// Create and Initialize the KTX2 transcoder object.
	basist::ktx2_transcoder transcoder;
	if (!transcoder.init(fileData.data(), fileData.size_u32())) {
		errs("Transcoder failed to initialize for file {}.", fileName.c_str());
	}

	const uint32 width = transcoder.get_width();
	const uint32 height = transcoder.get_height();
	const uint32 numMips = transcoder.get_levels();

	VkExtent3D imageSize;
	imageSize.width = width;
	imageSize.height = height;
	imageSize.depth = 1;

	msgs("Texture dimensions: ({}x{}), levels: {}", width, height, numMips);

	// Allocate image and transition to dst
	SImage_T* image = allocator.allocateImage(fileName, imageSize, VK_FORMAT_BC7_SRGB_BLOCK, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, numMips);

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		CVulkanUtils::transitionImage(cmd, image->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	});

	transcoder.start_transcoding();

	// Transcode each mipmap and add to vector
	for (uint32 mipmap = 0; mipmap < numMips; ++mipmap) {
		const auto level_width = basisu::maximum<uint32>(width >> mipmap, 1);
		const auto level_height = basisu::maximum<uint32>(height >> mipmap, 1);
		basisu::gpu_image tex(basisu::texture_format::cBC7, level_width, level_height); //cDecodeFlagsTranscodeAlphaDataToOpaqueFormats

		if (const bool status = transcoder.transcode_image_level(mipmap, 0, 0, tex.get_ptr(), tex.get_total_blocks(), basist::transcoder_texture_format::cTFBC7_RGBA, 0); !status) {
			errs("Image Transcode for file {} failed.", fileName.c_str());
		}

		const void* pImage = tex.get_ptr();
		const uint32 image_size = tex.get_size_in_bytes();

		VkExtent3D levelSize;
		levelSize.width = level_width;
		levelSize.height = level_height;
		levelSize.depth = 1;

		// Upload buffer is not needed outside of this function
		CVulkanResourceManager manager;
		const SBuffer_T* uploadBuffer = manager.allocateBuffer(image_size, VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		memcpy(uploadBuffer->GetMappedData(), pImage, image_size);

		CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
			//ZoneScopedAllocation(std::string("Copy Image from Upload Buffer"));

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = mipmap;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = levelSize;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadBuffer->buffer, image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		});

		manager.flush();
	}

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		CVulkanUtils::transitionImage(cmd, image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	return image;
}

using MeshData = std::vector<std::pair<std::string, std::shared_ptr<SMeshData>>>;

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
MeshData loadGLTF_Internal(CVulkanRenderer* renderer, std::filesystem::path path) {

	std::string fileName = path.filename().string();

	ZoneScoped;
	std::string string = "Loading: " + fileName;
	ZoneName(string.c_str(), string.size());

	msgs("Loading GLTF: {}", fileName.c_str());

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble;

	fastgltf::GltfFileStream data{path.string()};

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
		msgs("Invalid GLTF file : {}", path.string().c_str());
		return {};
	}

	// The meshes to save to a custom format
	MeshData savedMeshes;

	for (fastgltf::Node& node : gltf.nodes) {
		if (!node.meshIndex.has_value()) continue;
		fastgltf::Mesh& mesh = gltf.meshes[*node.meshIndex];

		auto outMesh = std::make_shared<SMeshData>();
		savedMeshes.push_back(std::pair<std::string, std::shared_ptr<SMeshData>>{node.name, outMesh});

		// Add the object
		//mLoadedModels.insert(outMesh);

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

			SStaticMesh::Surface newSurface;
			newSurface.material = renderer->mEngineTextures->mErrorMaterial;
			newSurface.startIndex = (uint32)indices.size();
			newSurface.count = (uint32)gltf.accessors[p.indicesAccessor.value()].count;

			SMeshData::Surface newSurface2;
			newSurface2.startIndex = (uint32)indices.size();
			newSurface2.count = (uint32)gltf.accessors[p.indicesAccessor.value()].count;

			if (p.materialIndex.has_value()) {
				newSurface.name = gltf.materials[p.materialIndex.value()].name;
			} else {
				newSurface.name = fmts("Surface {}", unnamedSurfaceIndex);
				unnamedSurfaceIndex++;
			}

			newSurface2.name = newSurface.name;

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

			// load vertex positions and bounds
			{
				fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
					[&](glm::vec3 v, size_t index) {
						SVertex newvtx;
						const Vector3f pos = localMatrix * Vector4f{v, 1.f};
						newvtx.position = pos;
						newvtx.normal = Vector3f{1, 0, 0};
						newvtx.setColor(Color4(0));
						newvtx.uv = 0;
						vertices[initial_vtx + index] = newvtx;
					});
			}

			// load vertex normals
			auto normals = p.findAttribute("NORMAL");
			if (normals < p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->accessorIndex],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv < p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->accessorIndex],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].setUV(v.x, v.y);
					});
			}

			// load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			outMesh->mHasVertexColor = colors < p.attributes.end();
			if (outMesh->mHasVertexColor) {
				// Sometimes the color will not have an alpha, in such a case, the alpha can be set to 1.f
				if (gltf.accessors[colors->accessorIndex].type == fastgltf::AccessorType::Vec3) {
					fastgltf::iterateAccessorWithIndex<Vector3f>(gltf, gltf.accessors[colors->accessorIndex],
						[&](Vector3f v, size_t index) {
							vertices[initial_vtx + index].setColor(Color4(v * 255.f, 255));
						});
				} else {
					fastgltf::iterateAccessorWithIndex<Vector4f>(gltf, gltf.accessors[colors->accessorIndex],
						[&](Vector4f v, size_t index) {
							vertices[initial_vtx + index].setColor(Color4(v * 255.f));
						});
				}
			}

			outMesh->surfaces.push_back(newSurface2);
		}

		// If no meshes are loaded, do not upload or add to mesh list
		if (indices.empty()) {
			return {};
		}

		optimizeMesh(indices, vertices);

		Vector3f minpos(std::numeric_limits<float>::max());
		Vector3f maxpos(std::numeric_limits<float>::min());

		for (auto& vertex : vertices) {
			minpos.x = glm::min(minpos.x, vertex.position.x);
			minpos.y = glm::min(minpos.y, vertex.position.y);
			minpos.z = glm::min(minpos.z, vertex.position.z);
			maxpos.x = glm::max(maxpos.x, vertex.position.x);
			maxpos.y = glm::max(maxpos.y, vertex.position.y);
			maxpos.z = glm::max(maxpos.z, vertex.position.z);
		}

		// Calculate origin and extents from the min/max, use extent length for radius
		outMesh->bounds.origin = (maxpos + minpos) / 2.f;
		outMesh->bounds.extents = (maxpos - minpos) / 2.f;
		outMesh->bounds.sphereRadius = glm::length(outMesh->bounds.extents);

		outMesh->indices = indices;
		outMesh->vertices = vertices;
	}

	msgs("GLTF {} Loaded.", path.string().c_str());

	return savedMeshes;
}

std::shared_ptr<SMeshData> readMeshData(const std::filesystem::path& path) {
	CFileArchive file(path.string(), "rb");

	if (!file.isOpen()) {
		msgs("Could not open Mesh file {}!", path.string().c_str());
		return {};
	}

	std::shared_ptr<SMeshData> outSaveData;

	file >> outSaveData;

	file.close();

	return outSaveData;
}

SMeshBuffers_T uploadMesh(CVulkanResourceManager& inManager, std::span<uint32> indices, std::span<SVertex> vertices) {
	const size_t vertexBufferSize = vertices.size() * sizeof(SVertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32);

	// Create buffers
	SMeshBuffers_T meshBuffers = inManager.allocateMeshBuffer(indexBufferSize, vertexBufferSize);

	// Staging is not needed outside of this function
	CVulkanResourceManager manager;
	const SBuffer_T* staging = manager.allocateBuffer(vertexBufferSize + indexBufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	//TODO: slow, render thread?
	// also from an older version of the tutorial, doesnt use sync2
	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, meshBuffers.vertexBuffer->buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{};
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, meshBuffers.indexBuffer->buffer, 1, &indexCopy);
	});

	manager.flush();

	return meshBuffers;
}

std::shared_ptr<SStaticMesh> toStaticMesh(const std::shared_ptr<SMeshData>& mesh, const std::string& fileName) {
	CVulkanRenderer& renderer = CEngine::renderer();

	auto loadMesh = std::make_shared<SStaticMesh>();
	loadMesh->name = fileName;
	loadMesh->bounds = mesh->bounds;
	for (auto& [name, startIndex, count] : mesh->surfaces) {
		loadMesh->surfaces.push_back({
			.name = name,
			.material = renderer.mEngineTextures->mErrorMaterial,
			.startIndex = startIndex,
			.count = count
		});
	}

	loadMesh->meshBuffers = uploadMesh(renderer.mGlobalResourceManager, mesh->indices, mesh->vertices);
	return loadMesh;
}

void CEngineLoader::save() {
	// Only materials need to be saved (as they are the only thing created at runtime)
	for (const auto& [name, material] : get().mMaterials) {
		save(name, material);
	}
}

void CEngineLoader::load() {

	basisu::basisu_encoder_init(gUseOpenCL, false);

	std::vector<std::filesystem::path> textures;
	std::vector<std::filesystem::path> materials;
	std::vector<std::filesystem::path> meshes;

	// Add each type to their respective vector
	for (std::filesystem::recursive_directory_iterator i(SPaths::get().mAssetPath), end; i != end; ++i) {
		if (!std::filesystem::is_directory(i->path())) {
			if (i->path().extension() == ".ktx2") {
				textures.push_back(i->path());
			} else if (i->path().extension() == ".mat") {
				materials.push_back(i->path());
			} else if (i->path().extension() == ".msh") {
				meshes.push_back(i->path());
			}
		}
	}

	auto pathToName = [](const std::filesystem::path& path) {
		std::filesystem::path fileName = path.filename();
		fileName.replace_extension();
		return fileName.string();
	};

	// Specific load order, since meshes reference materials, and materials reference textures

	// Load textures
	for (const auto& path : textures) {
		SImage_T* image = loadImage(CEngine::renderer().mGlobalResourceManager, path);
		get().mImages.emplace(pathToName(path), image);
	}

	// Load materials
	for (const auto& path : materials) {
		auto material = load<std::shared_ptr<CMaterial>>(path);
		get().mMaterials.emplace(pathToName(path), material);
	}

	// Load meshes
	for (const auto& path : meshes) {
		auto mesh = load<std::shared_ptr<SMeshData>>(path);
		get().mMeshes.emplace(pathToName(path), toStaticMesh(mesh, pathToName(path)));
	}
}

void CEngineLoader::importTexture(const std::filesystem::path& inPath) {
	const std::string fileName = inPath.filename().string();

	basisu::image image;
	basisu::load_image(inPath.string().c_str(), image);

	basisu::vector<basisu::image> images;
	images.push_back(image);

	size_t file_size = 0;
	constexpr uint32 quality = 255;

	// TODO cETC1S is supremely smaller, but takes significantly longer to compress. cETC1S should be the main format.
	void* pKTX2_data = basisu::basis_compress(
		basist::basis_tex_format::cUASTC4x4,
		images,
		basisu::cFlagGenMipsClamp | basisu::cFlagKTX2 | basisu::cFlagSRGB | basisu::cFlagThreaded | basisu::cFlagUseOpenCL | basisu::cFlagDebug | basisu::cFlagPrintStats | basisu::cFlagPrintStatus, 0.0f,
		&file_size,
		nullptr);//quality |

	if (!pKTX2_data) {
		errs("Compress for file {} failed.", fileName.c_str());
	}

	std::filesystem::path cachedPath = SPaths::get().mAssetPath.string() + fileName;
	cachedPath.replace_extension(".ktx2");

	// Write to ktx2 file
	if (!basisu::write_data_to_file(cachedPath.string().c_str(), pKTX2_data, file_size)) {
		errs("Ktx2 file {} could not be written.", fileName.c_str());
	}

	basisu::basis_free_data(pKTX2_data);

	SImage_T* loadedImage = loadImage(CEngine::renderer().mGlobalResourceManager, cachedPath);

	get().mImages.emplace(loadedImage->name, loadedImage);
}

void CEngineLoader::createMaterial(const std::string& inMaterialName) {
	// Get a name that isn't taken
	int32 materialNumber = 0;
	std::string name;
	bool contains = true;
	while (contains) {
		name = fmts("material {}", materialNumber);
		contains = false;
		for (const auto& material : CMaterial::getMaterials()) {
			if (material->mName == name) {
				contains = true;
				break;
			}
		}
		materialNumber++;
	}

	const auto material = std::make_shared<CMaterial>();
	material->mName = name;
	CMaterial::getMaterials().push_back(material);

	get().mMaterials.emplace(name, material);
}

void CEngineLoader::importMesh(const std::filesystem::path& inPath) {
	CVulkanRenderer& renderer = CEngine::renderer();

	const std::string fileName = inPath.filename().string();

	// Load the GLTF into Mesh Data
	const auto meshData = loadGLTF_Internal(&renderer, inPath);

	// If failed, do not write data
	if (meshData.empty()) {
		msgs("Mesh {} is empty!", fileName.c_str());
		return;
	}

	// Save the mesh data
	for (const auto& [fileName, data] : meshData) {

		// Create an asset path with the appropriate name
		std::filesystem::path path = SPaths::get().mAssetPath;
		path.append(fileName + ".msh");

		// Ensure .msh doesn't already exist
		if (std::filesystem::exists(path)) {
			msgs("File {} already exists!", fileName);
			return;
		}

		CFileArchive file(path.string(), "wb");

		file << data;

		file.close();

		const auto mesh = readMeshData(path);

		get().mMeshes.emplace(fileName, toStaticMesh(mesh, fileName));
	}
}
