#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl3.h"

#include "Common.h"
#include "EngineLoader.h"
#include "Material.h"
#include "Scene.h"
#include "Sprite.h"
#include "SpritePass.h"
#include "StaticMesh.h"
#include "Threading.h"
#include "VulkanRenderer.h"
#include "Viewport.h"

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

		CDescriptorPool* descriptorPool;
		renderer.mGlobalResourceManager.push(descriptorPool, poolCreateInfo);

		// this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo initInfo {
			.Instance = CEngine::instance(),
			.PhysicalDevice = CEngine::physicalDevice(),
			.Device = CEngine::device(),
			.Queue = inQueue,
			.DescriptorPool = *descriptorPool,
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

	static void renderSceneUI() {
		if (ImGui::Begin("Scene")) {
			CScene& scene = CScene::get();
			if (ImGui::Button("Add Mesh Object")) {
				scene.data.objects.push_back(std::make_shared<CStaticMeshObject>());
			}
			uint32 objectNum = 0;
			for (auto& object : scene.data.objects) {
				if (!object) continue;
				ImGui::PushID(fmts("{}", objectNum).c_str());

				if (ImGui::TreeNode("##xx")) {
					ImGui::SameLine();
					ImGui::InputText("Name", &object->mName);

					ImGui::InputFloat3("Position", reinterpret_cast<float*>(&object->mPosition));
					ImGui::InputFloat3("Rotation", reinterpret_cast<float*>(&object->mRotation));
					ImGui::InputFloat3("Scale", reinterpret_cast<float*>(&object->mScale));

					auto& sobject = dynamic_cast<CStaticMeshObject&>(*object);

					const char* combo_preview_value = sobject.getMesh() ? sobject.getMesh()->name.c_str() : "None";
					if (ImGui::BeginCombo("Meshes", combo_preview_value, ImGuiComboFlags_HeightRegular)) {
						for (auto& mesh : CEngineLoader::getMeshes()) {
							if (mesh.second->name.empty()) continue;

							const bool is_selected = ((sobject.getMesh() ? sobject.getMesh()->name : "None") == mesh.second->name);
							if (ImGui::Selectable(CEngineLoader::getMeshes()[mesh.first]->name.c_str(), is_selected)) {
								sobject.mesh = mesh.second;
								msgs("attempted to set {} to {}", sobject.mesh ? sobject.mesh->name.c_str() : "None", mesh.second->name.c_str());
							}


							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					ImGui::TreePop();
				} else {
					ImGui::SameLine();
					ImGui::InputText("Name", &object->mName);
				}
				objectNum++;
				ImGui::PopID();
			}
		}
		ImGui::End();
	}

	static void renderTextureUI() {
		if (ImGui::Begin("Textures")) {
			if (ImGui::Button("Import Texture")) {

				// Supported textures?
				const static std::vector<std::pair<const char*, const char*>> filters = {
					{ "Supported Formats (*.png; *.tga; *.jpg; *.jpeg)", "png;tga;jpg;jpeg" },
					{ "PNG (*.png)", "png" },
					{ "TGA (*.tga)", "tga" },
					{ "JPEG (*.jpg; *.jpeg)", "jpg;jpeg" }
				};

				//TODO: file query shouldnt be in viewport
				CEngineViewport::queryForFile(filters, [](std::vector<std::string> inFiles) {
					for (const auto& file : inFiles) {
						CThreading::runOnBackgroundThread([file] {
							CEngineLoader::importTexture(file);
						});
					}
				});
			}
		}
		ImGui::End();
	}

	static void renderMaterialUI() {
		static uint32 selected = 0;
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
					selected = std::min(static_cast<uint32>(CMaterial::getMaterials().size() - 1), selected);
				}
			}

			if (!CMaterial::getMaterials().empty()) {
				const std::shared_ptr<CMaterial> material = CMaterial::getMaterials()[selected];

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
			for (const auto& sprite : CEngine::renderer().mSpritePass->objects) {
				if (ImGui::BeginTabBar("Sprite")) {
					if (ImGui::BeginTabItem(sprite->mName.c_str())) {
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

				//TODO: file query shouldn't be in viewport
				CEngineViewport::queryForFile(filters, [](std::vector<std::string> inFiles) {
					for (const auto& file : inFiles) {
						CThreading::runOnBackgroundThread([file] {
							CEngineLoader::importMesh(file);
						});
					}
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
		renderSceneUI();
	}
}
