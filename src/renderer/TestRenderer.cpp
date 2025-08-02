#include "TestRenderer.h"

#include <fstream>
#include <ranges>

#include "Archive.h"
#include "Engine.h"
#include "GpuScene.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "MeshLoader.h"

#define BASISU_FORCE_DEVEL_MESSAGES 1
#include "MeshPass.h"
#include "encoder/basisu_enc.h"

constexpr static bool gUseOpenCL = true;

void CTestRenderer::init() {
	CVulkanRenderer::init();

	basisu::basisu_encoder_init(gUseOpenCL, false);

	//load meshes
	mGlobalResourceManager.loadImage("column_albedo.png");
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
	mGlobalResourceManager.loadImage("trim_misc_2_albedo.png");
	mMeshLoader->loadGLTF(this, "structure2.glb");
	mGPUScene->basePass->push(mMeshLoader->mLoadedModels);

	const std::string path = CEngine::get().mAssetPath + "materials.txt";
	if (CFileArchive inFile(path, "rb"); inFile.isOpen())
		inFile >> CMaterial::getMaterials();

	//CMaterial::getMaterials().push_back(mGPUScene->mErrorMaterial);
}

void CTestRenderer::destroy() {
	CVulkanRenderer::destroy();
	const std::string path = CEngine::get().mAssetPath + "materials.txt";
	CFileArchive outFile(path, "wb");
	outFile << CMaterial::getMaterials();
}

void CTestRenderer::render(VkCommandBuffer cmd) {

	ImGui::ShowDemoWindow();

	static int32 selected = 0;

	if (ImGui::Begin("Materials")) {
		ImGui::Text("Add Material");
		ImGui::SameLine();
		if (ImGui::SmallButton("+")) {
			// Get a name that isn't taken
			int32 materialNumber = 0;
			std::string testName;
			bool contains = true;
			while (contains) {

				testName = fmts("material {}", materialNumber);

				contains = false;
				for (const auto& material : CMaterial::getMaterials()) {
					if (material->mName == testName) {
						contains = true;
						break;
					}
				}

				materialNumber++;
			}

			const auto material = std::make_shared<CMaterial>();
			material->mName = testName;
			CMaterial::getMaterials().push_back(material);
			selected = CMaterial::getMaterials().size() - 1;
		}

		if (!CMaterial::getMaterials().empty()) {
			ImGui::SameLine();
			if (ImGui::SmallButton("-")) {
				CMaterial::getMaterials().erase(CMaterial::getMaterials().begin() + selected);
				selected = std::min(static_cast<int32>(CMaterial::getMaterials().size() - 1), selected);
			}
		}

		if (!CMaterial::getMaterials().empty()) {
			std::shared_ptr<CMaterial> material = CMaterial::getMaterials()[selected];

			if (material) {
				if (ImGui::BeginCombo("Material", material->mName.c_str(), ImGuiComboFlags_HeightRegular)) {
					for (int32 i = 0; i < CMaterial::getMaterials().size(); ++i) {
						const bool isSelected = selected == i;
						if (ImGui::Selectable(CMaterial::getMaterials()[i]->mName.c_str(), isSelected))
							selected = i;

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				ImGui::InputText("Name: ", &material->mName);

				const char* passTypePreviewValue = getMaterialPassToString(material->mPassType);
				if (ImGui::BeginCombo("Pass Type", passTypePreviewValue, ImGuiComboFlags_HeightRegular)) {
					for (uint8 n = 0; n < valueOf(EMaterialPass::MAX); n++) {
						const bool is_selected = material->mPassType == n;
						if (ImGui::Selectable(getMaterialPassToString(toMaterialPass(n)), is_selected))
							material->mPassType = toMaterialPass(n);

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				if (ImGui::TreeNode("Inputs")) {
					for (uint8 i = 0; i < 8; ++i) {
						ImGui::InputFloat4(fmts("Input {}", i).c_str(), (float*)&material->mConstants[i]);
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Code")) {
					ImGui::InputTextMultiline("##code", &material->mCode, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);
					ImGui::TreePop();
				}
			}
		}
	}
	ImGui::End();

	if (ImGui::Begin("Meshes")) {
		for (const auto& mesh : mMeshLoader->mLoadedModels) {
			if (ImGui::BeginTabBar("Mesh")) {
				if (ImGui::BeginTabItem(mesh->name.c_str())) {
					for (auto& surface : mesh->surfaces) {
						ImGui::PushID(surface.name.c_str());
						if (ImGui::CollapsingHeader(surface.name.c_str())) {
							//ImGui::Checkbox("Highlight", &material.material->mHighlight);
							//ImGui::InputInt("Color Texture ID", &material.second->data.colorTextureId);
							/*const char* combo_preview_value = mGlobalResourceManager.m_Images[surface.material->colorTextureId]->name.c_str();
							if (ImGui::BeginCombo("Color Texture", combo_preview_value, ImGuiComboFlags_HeightRegular)) {
								for (int n = 0; n < mGlobalResourceManager.m_Images.size(); n++) {
									if (mGlobalResourceManager.m_Images[n]->name.empty()) continue;

									const bool is_selected = (surface.material->colorTextureId == n);
									if (ImGui::Selectable(mGlobalResourceManager.m_Images[n]->name.c_str(), is_selected))
										surface.material->colorTextureId = n;

									// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}*/

							if (!CMaterial::getMaterials().empty()) {
								if (ImGui::BeginCombo("Surface Material", surface.material->mName.c_str(), ImGuiComboFlags_HeightRegular)) {
									for (const auto& material : CMaterial::getMaterials()) {
										const bool isSelected = surface.material == material;
										if (ImGui::Selectable(material->mName.c_str(), isSelected)) {
											surface.material = material;
										}

										// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
										if (isSelected)
											ImGui::SetItemDefaultFocus();
									}

									ImGui::EndCombo();
								}
							}

							if (ImGui::TreeNode("Material")) {
								ImGui::TreePop();
							}
						}
						ImGui::PopID();
					}
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

