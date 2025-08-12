#include "TestRenderer.h"

#include "Archive.h"
#include "Engine.h"
#include "VulkanDevice.h"
#include "imgui.h"

#define BASISU_FORCE_DEVEL_MESSAGES 1
#include "EngineTextures.h"
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

	constexpr int32 numSprites = 1000;

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution distribx(0, 100);
	std::uniform_int_distribution distriby(0, 100);

	const auto sprite = std::make_shared<CInstancedSprite>();
	sprite->mName = fmts("Instanced Sprite");
	sprite->material = mEngineTextures->mErrorMaterial;
	mSpritePass->push(sprite);

	for (int32 i = 0; i < numSprites; ++i) {
		Transform2f transform;
		transform.mPosition = {(float)distribx(gen) / 100.f, (float)distriby(gen) / 100.f};
		transform.mScale = {0.025f, 0.05f};
		sprite->addInstance(transform);
	}

	/*Transform2f transform;
	transform.mOrigin = {0.5f, 0.5f};
	transform.mPosition = {0.5f, 0.5f};
	transform.mScale = {0.5f, 1.0f};
	sprite->addInstance(transform);*/

	const std::string path = SPaths::get().mAssetCachePath.string() + "materials.txt";
	if (CFileArchive inFile(path, "rb"); inFile.isOpen())
		inFile >> CMaterial::getMaterials();
}

void CTestRenderer::destroy() {
	CVulkanRenderer::destroy();
	const std::string path = SPaths::get().mAssetCachePath.string() + "materials.txt";
	CFileArchive outFile(path, "wb");
	outFile << CMaterial::getMaterials();
}

void CTestRenderer::render(VkCommandBuffer cmd) {

}

