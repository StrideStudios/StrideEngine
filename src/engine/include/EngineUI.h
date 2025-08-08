#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl3.h"

#include "Common.h"
#include "EngineLoader.h"
#include "GpuScene.h"
#include "Material.h"
#include "SpritePass.h"
#include "Threading.h"
#include "VulkanRenderer.h"

namespace EngineUI {

	static void begin() {
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
	}

	static void init(VkQueue inQueue, VkFormat format) {
		CVulkanRenderer& renderer = CEngine::renderer();

		// Setup Dear ImGui context
		ImGui::CreateContext();

		// 1: create descriptor pool for IMGUI
		//  the size of the pool is very oversize, but it's copied from imgui demo
		//  itself.
		VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

		VkDescriptorPoolCreateInfo poolCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 1000,
			.poolSizeCount = (uint32)std::size(poolSizes),
			.pPoolSizes = poolSizes
		};

		const VkDescriptorPool descriptorPool = renderer.mGlobalResourceManager.allocateDescriptorPool(poolCreateInfo);

		// this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo initInfo {
			.Instance = CEngine::instance(),
			.PhysicalDevice = CEngine::physicalDevice(),
			.Device = CEngine::device(),
			.Queue = inQueue,
			.DescriptorPool = descriptorPool,
			.MinImageCount = 3,
			.ImageCount = 3,
			.UseDynamicRendering = true
		};

		//dynamic rendering parameters for imgui to use
		initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
		initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo);
		ImGui_ImplSDL3_InitForVulkan(CEngine::get().getViewport().mWindow);
	}

	static void destroy() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}

	static void render(VkCommandBuffer cmd, VkExtent2D inExtent, VkImageView inTargetImageView) {
		ImGui::Render();

		VkRenderingAttachmentInfo colorAttachment = CVulkanUtils::createAttachmentInfo(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = CVulkanUtils::createRenderingInfo(inExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	static void renderTextureUI() {
		if (ImGui::Begin("Textures")) {
			if (ImGui::Button("Import Texture")) {

				// Supported textures?
				const static std::vector<std::pair<const char*, const char*>> filters = {
					{ "PNG (*.png)", "png" }
				};

				//TODO: file query shouldnt be in viewport
				SEngineViewport::queryForFile(filters, [](const char* inFileName) {
					std::string fileName = inFileName;
					CThreading::runOnBackgroundThread([fileName] {
						if (!fileName.empty()) {
							CEngineLoader::importTexture(fileName);
						}
					});
				});
			}
		}
		ImGui::End();
	}

	static void renderMaterialUI() {
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
						for (uint8 i = 0; i < material->mConstants.size(); ++i) {
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
	}

	static void renderSpriteUI() {
		return;
		if (ImGui::Begin("Sprites")) {
			for (const auto& sprite : CEngine::renderer().mGPUScene->spritePass->objects) {
				if (ImGui::BeginTabBar("Sprite")) {
					if (ImGui::BeginTabItem(sprite->name.c_str())) {
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
							if (ImGui::BeginCombo("Surface Material", sprite->material->mName.c_str(), ImGuiComboFlags_HeightRegular)) {
								for (const auto& material : CMaterial::getMaterials()) {
									const bool isSelected = sprite->material == material;
									if (ImGui::Selectable(material->mName.c_str(), isSelected)) {
										sprite->material = material;
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
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	static void renderMeshUI() {
		if (ImGui::Begin("Meshes")) {
			if (ImGui::Button("Import Mesh")) {

				const static std::vector<std::pair<const char*, const char*>> filters = {
					{ "glTF 2.0 (*.gltf; *.glb)", "gltf;glb" }
				};

				//TODO: file query shouldnt be in viewport
				SEngineViewport::queryForFile(filters, [](const char* inFileName) {
					std::string fileName = inFileName;
					CThreading::runOnBackgroundThread([fileName] {
						if (!fileName.empty()) {
							CEngineLoader::importMesh(fileName);
						}
					});
				});
			}
			for (const auto& [fst, snd] : CEngineLoader::getMeshes()) {
				if (ImGui::BeginTabBar("Mesh")) {
					if (ImGui::BeginTabItem(snd->name.c_str())) {
						for (auto& surface : snd->surfaces) {
							ImGui::PushID(surface.name.c_str());
							if (ImGui::CollapsingHeader(surface.name.c_str())) {
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

	static void runEngineUI() {

		// Render Engine Settings
		CEngineSettings::render();

		renderTextureUI();
		renderMaterialUI();
		renderSpriteUI();
		renderMeshUI();
	}
}
