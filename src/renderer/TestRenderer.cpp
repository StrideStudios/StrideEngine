#include "TestRenderer.h"

#include "Archive.h"
#include "Engine.h"
#include "VulkanDevice.h"
#include "GpuScene.h"
#include "imgui.h"

#define BASISU_FORCE_DEVEL_MESSAGES 1
#include "MeshPass.h"
#include "Paths.h"
#include "Sprite.h"
#include "SpritePass.h"
#include "encoder/basisu_enc.h"

void CTestRenderer::init() {
	CVulkanRenderer::init();

	/*CFileArchive file(SPaths::get().mAssetCachePath.string() + "testMaterial.mat", "wb");

	std::vector<std::shared_ptr<CMaterial>> materials;

	auto mat = std::make_shared<CMaterial>();
	mat->mName = "Test material";
	materials.push_back(mat);

	file << materials;

	file.close();

	CFileArchive materialFile(SPaths::get().mAssetCachePath.string() + "testMaterial.mat", "rb");

	std::vector<std::shared_ptr<CMaterial>> readMaterials;

	materialFile >> readMaterials;

	materialFile.close();

	msgs("{}", readMaterials.at(0)->mName.c_str());

	CFileArchive sceneFile(SPaths::get().mAssetCachePath.string() + "testScene.scn", "wb");

	std::vector<std::shared_ptr<CSceneObject>> objects;

	auto mesh = std::make_shared<SStaticMesh>();
	mesh->name = "Test material";
	mesh->surfaces.push_back({
		.name = "test surface",
		.material = mat
	});
	objects.push_back(std::make_shared<CStaticMeshObject>(mesh));

	sceneFile << objects;

	sceneFile.close();*/

	//load meshes
	/*mGlobalResourceManager.loadImage("column_albedo.png");
	mGlobalResourceManager.loadImage("column_normal.png");
	mGlobalResourceManager.loadImage("floor_grate_wholes_albedo.png");
	mGlobalResourceManager.loadImage("light_shafts.png");
	mGlobalResourceManager.loadImage("set_asphalt_albedo.png");
	mGlobalResourceManager.loadImage("tile_anti_slip_albedo.png");
	mGlobalResourceManager.loadImage("tile_metal_pillars_albedo.png");
	mGlobalResourceManager.loadImage("tile_painted_gun_metal_albedo.png");
	mGlobalResourceManager.loadImage("tile_rivet_panels_albedo.png");
	mGlobalResourceManager.loadImage("tile_steel_albedo.png");
	mGlobalResourceManager.loadImage("tile_tech_panels_albedo.png");
	mGlobalResourceManager.loadImage("tile_tech_panels_color_albedo.png");
	mGlobalResourceManager.loadImage("trim_misc_1_albedo.png");
	mGlobalResourceManager.loadImage("trim_misc_2_albedo.png");*/
	//mMeshLoader->loadGLTF(this, "structure2.glb");
	//CThreading::getMainThread().executeAll(); //TODO: temp

	/*auto meshObject = std::make_shared<CStaticMeshObject>();
	meshObject->mesh = mMeshLoader->getMesh("structure2");
	meshObject->setPosition(Vector3d(0.0, 0.0, 0.0));
	mGPUScene->renderables.push_back(meshObject);*/

	auto material = std::make_shared<CMaterial>();
	material->mShouldSave = false;
	material->mName = "Test";
	material->mPassType = EMaterialPass::OPAQUE;

	constexpr int32 numSprites = 10000;

	struct InputData {
		Vector4f pos;
		uint32 textureID;
		Vector3f padding;
	};

	std::vector<InputData> spriteData;
	spriteData.resize(numSprites);

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution distribx(1, 1920);
	std::uniform_int_distribution distriby(1, 1080);

	//TODO: x value seems to not like being anything other than 0
	for (int32 i = 0; i < numSprites; ++i) {
		spriteData[i] = InputData({(float)distribx(gen), (float)distriby(gen), 20.f, 20.f}, i % 16);
	}
	size_t size = spriteData.size() * sizeof(InputData);

	SBuffer buffer = mGlobalResourceManager.allocateBuffer(size, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

	VkBufferDeviceAddressInfo deviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = buffer->buffer };
	VkDeviceAddress vertexBufferAddress = vkGetBufferDeviceAddress(CEngine::device(), &deviceAddressInfo);

	// Staging is not needed outside of this function
	CResourceManager manager;
	const SBuffer staging = manager.allocateBuffer(size, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();

	// copy vertex buffer
	memcpy(data, spriteData.data(), size);

	immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy copy{};
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = size;

		vkCmdCopyBuffer(cmd, staging->buffer, buffer->buffer, 1, &copy);
	});

	manager.flush();

	auto lowBits = (uint32) (vertexBufferAddress & 0xffffffffUL);
	auto highBits = (uint32) (vertexBufferAddress >> 32);

	material->mConstants[0] = {highBits, lowBits, 0.f, 0.f};

	auto sprite = std::make_shared<SSprite>();
	sprite->name = "Test Sprite";
	sprite->material = material;
	sprite->mInstances = numSprites;
	mGPUScene->spritePass->push(sprite);


	const std::string path = SPaths::get().mAssetPath.string() + "materials.txt";
	if (CFileArchive inFile(path, "rb"); inFile.isOpen())
		inFile >> CMaterial::getMaterials();

	//CMaterial::getMaterials().push_back(mGPUScene->mErrorMaterial);
}

void CTestRenderer::destroy() {
	CVulkanRenderer::destroy();
	const std::string path = SPaths::get().mAssetPath.string() + "materials.txt";
	CFileArchive outFile(path, "wb");
	outFile << CMaterial::getMaterials();
}

void CTestRenderer::render(VkCommandBuffer cmd) {

}

