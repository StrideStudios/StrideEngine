#include "renderer/passes/EngineUIPass.h"

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl3.h"

#include "engine/EngineSettings.h"
#include "renderer/VulkanRenderer.h"
#include "VkBootstrap.h"

#include "rendercore/EngineLoader.h"
#include "rendercore/Material.h"
#include "scene/base/Scene.h"
#include "renderer/passes/SpritePass.h"
#include "rendercore/StaticMesh.h"
#include "scene/world/StaticMeshObject.h"
#include "engine/Viewport.h"
#include "basic/core/Threading.h"
#include "engine/Engine.h"
#include "renderer/EngineTextures.h"
#include "rendercore/VulkanDevice.h"
#include "rendercore/VulkanInstance.h"
#include "scene/viewport/generic/Text.h"

void renderSceneUI(const SRendererInfo& info) {
	if (ImGui::Begin("Scene")) {
		const TShared<CScene> scene = CScene::get();
		if (ImGui::Button("Add Mesh Object")) {
			//scene.addChild(CStaticMeshObject{});
			TUnique<CStaticMeshObject> obj{};
			scene->addChild(std::move(obj));
		}
		for (size_t objectNum = 0; objectNum < scene->getChildren().getSize(); ++objectNum) {
			auto& object = scene.get()->getChildren()[objectNum];
			if (!object) continue;
			ImGui::PushID(fmts("{}", objectNum).c_str());

			if (ImGui::TreeNode("##xx")) {
				ImGui::SameLine();
				ImGui::InputText("Name", &object->mName);

				if (ImGui::Button("Remove")) {
					scene->removeChild(object);
				} else {
					Vector3f position = object->getPosition();
					ImGui::InputFloat3("Position", reinterpret_cast<float*>(&position));
					if (position != object->getPosition()) object->setPosition(position);
					Vector3f rotation = object->getRotation();
					ImGui::InputFloat3("Rotation", reinterpret_cast<float*>(&rotation));
					if (rotation != object->getRotation()) object->setRotation(rotation);
					Vector3f scale = object->getScale();
					ImGui::InputFloat3("Scale", reinterpret_cast<float*>(&scale));
					if (scale != object->getScale()) object->setScale(scale);

					if (auto sobject = dynamic_cast<CStaticMeshObject*>(object.get())) {
						const char* combo_preview_value = sobject->getMesh() ? sobject->getMesh()->name.c_str() : "None";
						if (ImGui::BeginCombo("Meshes", combo_preview_value, ImGuiComboFlags_HeightRegular)) {
							for (auto& mesh : CEngineLoader::getMeshes()) {
								if (mesh.second->name.empty()) continue;

								const bool is_selected = ((sobject->getMesh() ? sobject->getMesh()->name : "None") == mesh.second->name);
								if (ImGui::Selectable(CEngineLoader::getMeshes()[mesh.first]->name.c_str(), is_selected)) {
									sobject->mesh = mesh.second;
									msgs("attempted to set {} to {}", sobject->mesh ? sobject->mesh->name.c_str() : "None", mesh.second->name.c_str());
								}


								// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}
				}
				ImGui::TreePop();
			} else {
				ImGui::SameLine();
				ImGui::InputText("Name", &object->mName);
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}

void renderFontUI(const SRendererInfo& info) {
	if (ImGui::Begin("Fonts")) {
		if (ImGui::Button("Import Font")) {

			// Supported fonts
			const static std::vector<std::pair<const char*, const char*>> filters = {
				{ "Supported Formats (*.ttf; *.otf;)", "ttf;otf" },
				{ "TTF (*.ttf)", "ttf" },
				{ "OTF (*.otf)", "otf" }
			};

			//TODO: file query shouldnt be in viewport (also should be on background, but rendering thread crashes)
			CEngineViewport::queryForFile(filters, [&](std::vector<std::string> inFiles) {
				for (const auto& file : inFiles) {
					CThreading::getMainThread().add([info, file] {
						CEngineLoader::importFont(info.allocator, file);
					});
				}
			});
		}

		for (auto font : CEngineLoader::getFonts()) {
			if (ImGui::BeginCombo("Font", font.first.c_str(), ImGuiComboFlags_HeightRegular)) {
				static uint32 selected = 0;
				for (int32 i = 0; i < CEngineLoader::getFonts().size(); ++i) {
					const bool isSelected = selected == i;
					if (ImGui::Selectable(font.first.c_str(), isSelected))
						selected = i;

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		}
	}
	ImGui::End();
}

void renderTextureUI(const SRendererInfo& info) {
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
			CEngineViewport::queryForFile(filters, [&](std::vector<std::string> inFiles) {
				for (const auto& file : inFiles) {
					CThreading::runOnBackgroundThread([info, file] {
						CEngineLoader::importTexture(info.allocator, file);
					});
				}
			});
		}
	}
	ImGui::End();
}

void renderMaterialUI(const SRendererInfo& info) {
	static std::string selected = "";
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
				for (const auto& material : CEngineLoader::getMaterials()) {
					if (material.second->mName == testName) {
						contains = true;
						break;
					}
				}

				materialNumber++;
			}

			CMaterial* material;
			CResourceManager::get().create(material);
			material->mName = testName;
			CEngineLoader::getMaterials().emplace(testName, material);
			selected = testName;
		}

		if (!CEngineLoader::getMaterials().empty()) {
			ImGui::SameLine();
			if (ImGui::SmallButton("-")) {
				auto itr = std::prev(CEngineLoader::getMaterials().find(selected));
				selected = itr == CEngineLoader::getMaterials().end() ? "" : itr->first;
				CEngineLoader::getMaterials().erase(selected);
			}
		}

		if (!CEngineLoader::getMaterials().empty()) {
			CMaterial* material = CEngineLoader::getMaterials()[selected];

			if (material) {
				if (ImGui::BeginCombo("Material", material->mName.c_str(), ImGuiComboFlags_HeightRegular)) {
					for (auto itr = CEngineLoader::getMaterials().begin(); itr != CEngineLoader::getMaterials().end(); ++itr) {
						const bool isSelected = selected == itr->first;
						if (ImGui::Selectable(CEngineLoader::getMaterials()[itr->first]->mName.c_str(), isSelected))
							selected = itr->first;

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

void renderSpriteUI(const SRendererInfo& info) {
	return;
	if (ImGui::Begin("Sprites")) {
		for (const auto& pass = CRenderer::get()->getPass<CSpritePass>(); const auto& sprite : pass->objects) {
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



					if (!CEngineLoader::getMaterials().empty()) {
						if (ImGui::BeginCombo("Surface Material", sprite->getMaterial()->mName.c_str(), ImGuiComboFlags_HeightRegular)) {
							for (const auto& material : CEngineLoader::getMaterials()) {
								const bool isSelected = sprite->getMaterial() == material.second;
								if (ImGui::Selectable(material.first.c_str(), isSelected)) {
									//sprite->material = material.second;
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

void renderMeshUI(const SRendererInfo& info) {
	if (ImGui::Begin("Meshes")) {
		if (ImGui::Button("Import Mesh")) {

			const static std::vector<std::pair<const char*, const char*>> filters = {
				{ "glTF 2.0 (*.gltf; *.glb)", "gltf;glb" }
			};

			//TODO: file query shouldn't be in viewport
			CEngineViewport::queryForFile(filters, [&](std::vector<std::string> inFiles) {
				for (const auto& file : inFiles) {
					CThreading::runOnBackgroundThread([info, file] {
						CEngineLoader::importMesh(info.allocator, file);
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
							if (!CEngineLoader::getMaterials().empty()) {
								if (ImGui::BeginCombo("Surface Material", surface.material->mName.c_str(), ImGuiComboFlags_HeightRegular)) {
									for (const auto& material : CEngineLoader::getMaterials()) {
										const bool isSelected = surface.material == material.second;
										if (ImGui::Selectable(material.first.c_str(), isSelected)) {
											surface.material = material.second;
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

void CEngineUIPass::init() {
	CPass::init();

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

	imguiPool = TUnique<CDescriptorPool>{poolCreateInfo};

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo initInfo {
		.Instance = CVulkanInstance::instance(),
		.PhysicalDevice = CVulkanDevice::physicalDevice(),
		.Device = CVulkanDevice::device(),
		.Queue = CVulkanDevice::get()->getQueue(EQueueType::GRAPHICS).mQueue,
		.DescriptorPool = imguiPool->get(),
		.MinImageCount = 3,
		.ImageCount = 3,
		.UseDynamicRendering = true
	};

	const VkFormat format = CVulkanRenderer::get()->mEngineTextures->mDrawImage->getFormat();

	//dynamic rendering parameters for imgui to use
	initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
	initInfo.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo);
	ImGui_ImplSDL3_InitForVulkan(CEngine::get()->getViewport()->mWindow);

	msgs("INIT UI PASS");
}

void CEngineUIPass::begin() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void CEngineUIPass::render(const SRendererInfo& info, VkCommandBuffer cmd) {
	// Render Engine Settings
	CEngineSettings::render();

	renderTextureUI(info);
	renderMaterialUI(info);
	renderSpriteUI(info);
	renderMeshUI(info);
	renderSceneUI(info);
	renderFontUI(info);

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void CEngineUIPass::destroy() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	imguiPool->destroy();
	ImGui::DestroyContext();

	msgs("DESTROY UI PASS");

}