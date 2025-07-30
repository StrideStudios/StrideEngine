#include "TestRenderer.h"

#include "GpuScene.h"
#include "imgui.h"
#include "MeshLoader.h"

void CTestRenderer::init() {
	CVulkanRenderer::init();

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
	mGPUScene->basePass.push(mMeshLoader->mLoadedModels);
}

void CTestRenderer::render(VkCommandBuffer cmd) {
	if (ImGui::Begin("Meshes")) {
		for (const auto& mesh : mMeshLoader->mLoadedModels) {
			if (ImGui::BeginTabBar("Mesh")) {
				if (ImGui::BeginTabItem(mesh->name.c_str())) {
					for (auto surface : mesh->surfaces) {
						ImGui::PushID(surface.name.c_str());
						if (ImGui::CollapsingHeader(surface.name.c_str())) {
							//ImGui::Checkbox("Highlight", &material.material->mHighlight);
							//ImGui::InputInt("Color Texture ID", &material.second->data.colorTextureId);
							const char* combo_preview_value = mGlobalResourceManager.m_Images[surface.material->colorTextureId]->name.c_str();
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

