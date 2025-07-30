#include "ShaderCompiler.h"

#include <filesystem>
#include <fstream>

#include "BasicTypes.h"
#include "Common.h"
#include "Engine.h"
#include "Hashing.h"
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
	FILE* file;
	fopen_s(&file, inFileName, "r");

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
		const auto p1 = code.find("\"", pos);
		const auto p2 = code.find("\"", p1 + 1);
		if (p1 == code.npos || p2 == code.npos || p2 <= p1)
		{
			printf("Error while loading shader program: %s\n", code.c_str());
			return std::string();
		}
		const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
		const std::string include = readShaderFile((CEngine::get().mShaderPath + name.c_str()).c_str());
		code.replace(pos, p2-pos+1, include.c_str());
	}

	return code;
}

bool loadShader(VkDevice inDevice, const char* inFileName, uint32 Hash, SShader& inoutShader) {
	// Make sure the file exists before attempting to open
	if (!std::filesystem::exists(inFileName)) {
		return false;
	}

	// Open the file. With cursor at the end
	std::ifstream file(inFileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	// Find what the size of the file is by looking up the location of the cursor
	// Because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = file.tellg();

	// SPIRV expects the buffer to be on uint32, so make sure to reserve a int
	// Vector big enough for the entire file
	std::vector<uint32> buffer(fileSize / sizeof(uint32));

	// Put file cursor at beginning
	file.seekg(0);

	// Load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	// Now that the file is loaded into the buffer, we can close it
	file.close();

	// The first uint32 value is the hash, if it does not equal the hash for the shader code, it means the shader has changed
	if (buffer[0] != Hash) {
		msgs("Shader file {} has changed, recompiling.", inFileName);
		return false;
	}

	// Remove the hash so it doesnt mess up the SPIRV shader
	buffer.erase(buffer.begin());

	// Create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		// CodeSize has to be in bytes, so multply the ints in the buffer by size of
		.codeSize = buffer.size() * sizeof(uint32),
		.pCode = buffer.data()
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
	std::ofstream file(inFileName, std::ios::binary | std::ios::trunc);

	// Make sure the file is open
	if (!file.is_open()) {
		return false;
	}

	std::vector<uint32> data = inShader.mCompiledShader;

	// Add the hash to the first part of the shader
	data.insert(data.begin(), Hash);

	// write the data to the file
	file.write((char*)data.data(), data.size()*sizeof(uint32));

	// Close the file
	file.close();

	msgs("Compiled Shader {}.", inFileName);

	return true;
}

VkResult CShaderCompiler::getShader(VkDevice inDevice, const char* inFileName, SShader& inoutShader) {

	const std::string path = CEngine::get().mShaderPath + inFileName;
	const std::string SPIRVpath = path + ".spv";

	// Get the hash of the original source file so we know if it changed
	const auto shaderSource = readShaderFile(path.c_str());

	if (shaderSource.empty()) {
		errs("Nothing found in Shader file {}!", inFileName, SPIRVpath.c_str());
	}

	const uint32 Hash = CHashing::getHash(shaderSource);
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
