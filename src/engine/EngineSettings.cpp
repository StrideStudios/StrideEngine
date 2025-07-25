#include "EngineSettings.h"

#include "Engine.h"
#include "VulkanDevice.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "VulkanUtils.h"

void CEngineSettings::render(VkCommandBuffer cmd, VkExtent2D inExtent, VkImageView inTargetImageView) {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("Engine Settings")) {
		if (ImGui::BeginTabBar("Settings")) {
			for (auto&[category, commands] : CCommand::getCategoryMap()) {
				if (ImGui::BeginTabItem(category.c_str())) {
					for (auto& command : commands) {
						command->render();
					}
					ImGui::EndTabItem();
				}
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	ImGui::Render();

	VkRenderingAttachmentInfo colorAttachment = CVulkanUtils::createAttachmentInfo(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = CVulkanUtils::createRenderingInfo(inExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void CEngineSettings::init(VkQueue inQueue, VkDescriptorPool inPool, VkFormat format) {
	// Setup Dear ImGui context
	ImGui::CreateContext();

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo initInfo {
		.Instance = CEngine::instance(),
		.PhysicalDevice = CEngine::physicalDevice(),
		.Device = CEngine::device(),
		.Queue = inQueue,
		.DescriptorPool = inPool,
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
	ImGui_ImplSDL3_InitForVulkan(CEngine::get().getWindow().mWindow);
}

void CEngineSettings::destroy() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}