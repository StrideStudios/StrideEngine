create_module(StrideEngine-renderer PUBLIC SHARED
        glslang::glslang-default-resource-limits
        glslang::glslang
        fastgltf::fastgltf
        meshoptimizer
        basisu_encoder

        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-engine
        StrideEngine-scene
        StrideEngine-imgui
)

# Basis universal doesn't include itself by default
target_include_directories(StrideEngine-renderer PUBLIC
        "../../third_party/basis_universal"
)