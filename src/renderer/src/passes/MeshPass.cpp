#include "passes/MeshPass.h"

#include "renderer/EngineTextures.h"
#include "renderer/VulkanDevice.h"
#include "renderer/VulkanRenderer.h"
#include "StaticMesh.h"
#include "Profiling.h"
#include "EngineSettings.h"
#include "base/Scene.h"
#include "renderer/EngineLoader.h"
#include "renderer/object/StaticMeshObjectRenderer.h"

#define SETTINGS_CATEGORY "Rendering"
ADD_TEXT(Meshes, "Meshes: ");
ADD_TEXT(Drawcalls, "Draw Calls: ");
ADD_TEXT(Vertices, "Vertices: ");
ADD_TEXT(Triangles, "Triangles: ");
#undef SETTINGS_CATEGORY

//TODO: for now this is hard coded base pass, dont need anything else for now
void CMeshPass::init() {
	CPass::init();

	passType = EMeshPass::BASE_PASS; //Mesh Pass as base type, base pass implements instead (or maybe parent with pipelines)

	CVulkanRenderer& renderer = *CVulkanRenderer::get();

	CVulkanResourceManager manager;

	const SShader frag = manager.getShader("material\\mesh.frag");
	const SShader errorFrag = manager.getShader("material\\mesh_error.frag");
	const SShader vert = manager.getShader("material\\mesh.vert");

	const SShader basicFrag = manager.getShader("material\\basic.frag");
	const SShader wireframeVert = manager.getShader("material\\wireframe.vert");

	SPipelineCreateInfo createInfo {
		.vertexModule = *vert.mModule,
		.fragmentModule = *frag.mModule,
		.mColorFormat = renderer.mEngineTextures->mDrawImage->mImageFormat,
		.mDepthFormat = renderer.mEngineTextures->mDepthImage->mImageFormat
	};

	//TODO: could probably read from shader and do automatically...
	CVertexAttributeArchive attributes;
	attributes.createBinding(VK_VERTEX_INPUT_RATE_VERTEX);
	attributes << VK_FORMAT_R32G32B32_SFLOAT;// vec3 position
	attributes << VK_FORMAT_R32_UINT; // uint UV
	attributes << VK_FORMAT_R32G32B32_SFLOAT; // vec3 normal
	attributes << VK_FORMAT_R32_UINT;// uint color
	attributes.createBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;// mat4 Transform
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;

	opaquePipeline = CVulkanResourceManager::get().allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	// Transparent should be additive and always render in front
	createInfo.mBlendMode = EBlendMode::ADDITIVE;
	createInfo.mDepthTestMode = EDepthTestMode::FRONT;

	transparentPipeline = CVulkanResourceManager::get().allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	createInfo.fragmentModule = *errorFrag.mModule;
	createInfo.mBlendMode = EBlendMode::NONE;
	createInfo.mDepthTestMode = EDepthTestMode::NORMAL;

	errorPipeline = CVulkanResourceManager::get().allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	createInfo.vertexModule = *wireframeVert.mModule;
	createInfo.fragmentModule = *basicFrag.mModule;
	createInfo.mTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.mPolygonMode = VK_POLYGON_MODE_LINE;
	createInfo.mCullMode = VK_CULL_MODE_NONE;
	createInfo.mLineWidth = 5.f;

	wireframePipeline = CVulkanResourceManager::get().allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	manager.flush();
}

//TODO: probably faster with gpu
bool isVisible(const Matrix4f& inViewProj, const Matrix4f& inTransformMatrix, const SBounds& inBounds) {
	constexpr static std::array corners {
		Vector3f { 1, 1, 1 },
		Vector3f { 1, 1, -1 },
		Vector3f { 1, -1, 1 },
		Vector3f { 1, -1, -1 },
		Vector3f { -1, 1, 1 },
		Vector3f { -1, 1, -1 },
		Vector3f { -1, -1, 1 },
		Vector3f { -1, -1, -1 },
	};

	const Matrix4f matrix = inViewProj * inTransformMatrix;

	auto min = Vector3f{std::numeric_limits<float>::max()};
	auto max = Vector3f{std::numeric_limits<float>::min()};

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		Vector4f v = matrix * Vector4f(inBounds.origin + (corners[c] * inBounds.extents), 1.f);

		// perspective correction
		v.x = v.x / v.w;
		v.y = v.y / v.w;
		v.z = v.z / v.w;

		min = glm::min(Vector3f { v.x, v.y, v.z }, min);
		max = glm::max(Vector3f { v.x, v.y, v.z }, max);
	}

	// check the clip space box is within the view
	if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
		return false;
	}
	return true;
}

void CMeshPass::render(const VkCommandBuffer cmd) {
	ZoneScopedN("Base Pass");

	CVulkanRenderer& renderer = *CVulkanRenderer::get();

	std::vector<std::shared_ptr<CStaticMeshObject>> renderObjects;
	{
		ZoneScopedN("Frustum Culling");
		for (const auto& renderable : CScene::get().data.objects) {
			if (renderable) {
				if (auto renderableObject = std::dynamic_pointer_cast<CStaticMeshObject>(renderable); renderableObject && renderableObject->getMesh() && isVisible(renderer.mSceneData.mViewProj, renderableObject->getTransformMatrix(), renderableObject->getMesh()->bounds)) {
					renderObjects.push_back(renderableObject);
				}
			}
		}
	}

	// Set number of meshes being drawn
	Meshes.setText(fmts("Meshes: {}", renderObjects.size()));

	// Defined outside the draw function, this is the state we will try to skip
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	for (auto& object : renderObjects) {
		CStaticMeshObjectRenderer srenderer;
		srenderer.render(this, cmd, object.get(), drawCallCount, vertexCount);
	}

	// Set number of drawcalls, vertices, and triangles
	Drawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	Vertices.setText(fmts("Vertices: {}", vertexCount));
	Triangles.setText(fmts("Triangles: {}", vertexCount / 3));
}
