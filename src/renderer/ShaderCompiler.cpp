#include "ShaderCompiler.h"

#include <fstream>

#include "BasicTypes.h"
#include "Common.h"
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"
#include "glslang/Public/ShaderLang.h"

static uint32 compile(SShader& inoutShader) {

	glslang_stage_t stage;
	switch (inoutShader.mStage) {
		case EShaderStage::VERTEX:
			stage = GLSLANG_STAGE_VERTEX;
			break;
		case EShaderStage::PIXEL:
			stage = GLSLANG_STAGE_FRAGMENT;
			break;
		case EShaderStage::COMPUTE:
			stage = GLSLANG_STAGE_COMPUTE;
			break;
		default:
			return 0;
	}

	const glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_3,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_6,
		.code = inoutShader.mShaderCode.c_str(),
		.default_version = 460,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = glslang_default_resource(),
	};

	glslang_initialize_process();

	glslang_shader_t* shader = glslang_shader_create(&input);

	if (!glslang_shader_preprocess(shader, &input)) {
		err("Error Processing Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	if (!glslang_shader_parse(shader, &input)) {
		err("Error Parsing Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		err("Error Linking Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	glslang_program_SPIRV_generate(program, input.stage);

	inoutShader.mCompiledShader.resize(glslang_program_SPIRV_get_size(program));
	glslang_program_SPIRV_get(program, inoutShader.mCompiledShader.data());

	if (glslang_program_SPIRV_get_messages(program)) {
		msg("{}", glslang_program_SPIRV_get_messages(program));
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return inoutShader.mCompiledShader.size();
}

std::string readShaderFile(const char* inFileName) {
	FILE* file = fopen(inFileName, "r");

	if (!file)
	{
		printf("I/O error. Cannot open shader file '%s'\n", inFileName);
		return std::string();
	}

	fseek(file, 0L, SEEK_END);
	const auto bytesinfile = ftell(file);
	fseek(file, 0L, SEEK_SET);

	char* buffer = (char*)alloca(bytesinfile + 1);
	const size_t bytesread = fread(buffer, 1, bytesinfile, file);
	fclose(file);

	buffer[bytesread] = 0;

	// Remove the BOM at the beginning of the file that causes the compiler to miss the #version specifier
	static constexpr unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };

	if (bytesread > 3)
	{
		if (!memcmp(buffer, BOM, 3))
			memset(buffer, ' ', 3);
	}

	std::string code(buffer);

	// Process includes
	while (code.find("#include ") != code.npos)
	{
		const auto pos = code.find("#include ");
		const auto p1 = code.find('<', pos);
		const auto p2 = code.find('>', pos);
		if (p1 == code.npos || p2 == code.npos || p2 <= p1)
		{
			printf("Error while loading shader program: %s\n", code.c_str());
			return std::string();
		}
		const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
		const std::string include = readShaderFile(name.c_str());
		code.replace(pos, p2-pos+1, include.c_str());
	}

	return code;
}

VkResult CShaderCompiler::compileShader(VkDevice inDevice, const char* inFileName, SShader& inoutShader) {
	uint32 result = 0;
	if (const auto shaderSource = readShaderFile(inFileName); !shaderSource.empty()) {
		inoutShader.mShaderCode = shaderSource;
		result = compile(inoutShader);
	}

	// If we get to this point without throwing some kind of other error, it means the shader file could not be found
	if (!result) {
		err("Shader file {} not found!", inFileName);
	}

	const VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = inoutShader.mCompiledShader.size() * sizeof(inoutShader.mCompiledShader[0]),
		.pCode = inoutShader.mCompiledShader.data()
	};

	return vkCreateShaderModule(inDevice, &createInfo, nullptr, &inoutShader.mModule);
}
