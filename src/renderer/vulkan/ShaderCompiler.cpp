#include "ShaderCompiler.h"

#include <filesystem>
#include <fstream>

#include "Archive.h"
#include "Common.h"
#include "Engine.h"
#include "Hashing.h"
#include "Paths.h"
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
		errs("Error Processing Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	if (!glslang_shader_parse(shader, &input)) {
		errs("Error Parsing Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		errs("Error Linking Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	glslang_program_SPIRV_generate(program, input.stage);

	inoutShader.mCompiledShader.resize(glslang_program_SPIRV_get_size(program));
	glslang_program_SPIRV_get(program, inoutShader.mCompiledShader.data());

	if (glslang_program_SPIRV_get_messages(program)) {
		msgs("{}", glslang_program_SPIRV_get_messages(program));
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return inoutShader.mCompiledShader.size();
}

std::string readShaderFile(const char* inFileName) {
	CFileArchive file(inFileName, "r");

	if (!file.isOpen()) {
		printf("I/O error. Cannot open shader file '%s'\n", inFileName);
		return std::string();
	}

	std::string code = file.readFile(true);

	// Process includes
	while (code.find("#include ") != code.npos)
	{
		const auto pos = code.find("#include ");
		const auto p1 = code.find("\"", pos);
		const auto p2 = code.find("\"", p1 + 1);
		if (p1 == code.npos || p2 == code.npos || p2 <= p1)
		{
			printf("Error while loading shader program: %s\n", code.c_str());
			return std::string();
		}
		const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
		const std::string include = readShaderFile((SPaths::get().mShaderPath.string() + name.c_str()).c_str());
		code.replace(pos, p2-pos+1, include.c_str());
	}

	return code;
}

bool loadShader(VkDevice inDevice, const char* inFileName, uint32 Hash, SShader& inoutShader) {
	CFileArchive file(inFileName, "rb");

	if (!file.isOpen()) {
		return false;
	}

	std::vector<uint32> code = file.readFile<uint32>();

	// The first uint32 value is the hash, if it does not equal the hash for the shader code, it means the shader has changed
	if (code[0] != Hash) {
		msgs("Shader file {} has changed, recompiling.", inFileName);
		return false;
	}

	// Remove the hash so it doesnt mess up the SPIRV shader
	code.erase(code.begin());

	// Create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		// CodeSize has to be in bytes, so multply the ints in the buffer by size of
		.codeSize = code.size() * sizeof(uint32),
		.pCode = code.data()
	};

	// Check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(inDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}
	inoutShader.mModule = shaderModule;
	return true;
}

bool saveShader(const char* inFileName, uint32 Hash, const SShader& inShader) {
	CFileArchive file(inFileName, "wb");

	// Make sure the file is open
	if (!file.isOpen()) {
		return false;
	}

	std::vector<uint32> data = inShader.mCompiledShader;

	// Add the hash to the first part of the shader
	data.insert(data.begin(), Hash);

	file.writeFile(data);

	msgs("Compiled Shader {}.", inFileName);

	return true;
}

VkResult CShaderCompiler::getShader(VkDevice inDevice, const char* inFileName, SShader& inoutShader) {

	const std::string path = SPaths::get().mShaderPath.string() + inFileName;
	const std::string SPIRVpath = path + ".spv";

	// Get the hash of the original source file so we know if it changed
	const auto shaderSource = readShaderFile(path.c_str());

	if (shaderSource.empty()) {
		errs("Nothing found in Shader file {}!", inFileName);
	}

	const uint32 Hash = getHash(shaderSource);
	if (Hash == 0) {
		errs("Hash from file {} is not valid.", inFileName);
	}

	// Check for written SPIRV files
	if (loadShader(inDevice, SPIRVpath.c_str(), Hash, inoutShader)) {
		return VK_SUCCESS;
	}

	inoutShader.mShaderCode = shaderSource;
	const uint32 result = compile(inoutShader);
	// Save compiled shader
	if (!saveShader(SPIRVpath.c_str(), Hash, inoutShader)) {
		errs("Shader file {} failed to save to {}!", inFileName, SPIRVpath.c_str());
	}

	// This means the shader didn't compile properly
	if (!result) {
		errs("Shader file {} failed to compile!", inFileName);
	}

	if (loadShader(inDevice, SPIRVpath.c_str(), Hash, inoutShader)) {
		return VK_SUCCESS;
	}

	// Not sure what error to use, just use this error
	return VK_ERROR_INVALID_SHADER_NV;
}
