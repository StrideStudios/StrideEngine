
# Make sure files can pull from include paths
include_directories("core/include")
include_directories("vulkan/include")

add_executable(game
        main.cpp
        vulkan/include/VulkanDevice.h
        vulkan/VulkanDevice.cpp
        core/include/Common.h
        vulkan/VulkanRenderer.cpp
        vulkan/include/VulkanRenderer.h
        vulkan/include/VulkanUtils.h
        core/include/DeletionQueue.h
)

find_package(Vulkan REQUIRED)
target_link_libraries(game PRIVATE
        glfw::glfw
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        vma:vma
)