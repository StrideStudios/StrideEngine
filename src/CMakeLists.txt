add_executable(game
    main.cpp
)

target_link_libraries(game PRIVATE glfw::glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(game PRIVATE
    Vulkan::Vulkan
    vk-bootstrap::vk-bootstrap
)