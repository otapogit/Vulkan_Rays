#include "core_utils.h"
#include "core_shader.h"
#include <stdio.h>
#include <cassert>
#include <iostream>
#include <string>
//#include "utils.cpp"
#include <vector>
#include <fstream>
#include <string>

#include <glslang/Include/glslang_c_interface.h>

// Required for use of glslang_default_resource
#include <glslang/Public/resource_limits_c.h>

namespace core {
	
	struct coreShader {
		std::vector<uint32_t> SPIRV;
		VkShaderModule ShaderModule = NULL;
	
		void Initialize(glslang_program_t* program) {
			size_t program_size = glslang_program_SPIRV_get_size(program);
			SPIRV.resize(program_size);
			glslang_program_SPIRV_get(program, SPIRV.data());
		}
	};


	bool Read_pFile(const char* pFilename, std::string& outFile) {
		std::ifstream f(pFilename);


		bool ret = false;

		if (f.is_open()) {
			std::string line;
			while (std::getline(f, line)) {
				outFile.append(line);
				outFile.append("\n");
			}
			f.close();
			ret = true;
		}
		else {
			fprintf(stderr, "Error opening file %s", pFilename);
			exit(0);
		}
		return ret;
	}

	bool endsWith(const std::string& fullString,
		const std::string& ending)
	{
		// Check if the ending string is longer than the full
		// string
		if (ending.size() > fullString.size())
			return false;

		// Compare the ending of the full string with the target
		// ending
		return fullString.compare(fullString.size()
			- ending.size(),
			ending.size(), ending)
			== 0;
	}

	static glslang_stage_t ShaderStageFromFilename(const char* pFilename) {
		//a lo mejor hay que poner mas
		std::string s(pFilename);
		if (endsWith(s,".vert")) {
			return GLSLANG_STAGE_VERTEX;
		}
		if (endsWith(s, ".frag")) {
			return GLSLANG_STAGE_FRAGMENT;
		}
		if (endsWith(s, ".geom")) {
			return GLSLANG_STAGE_GEOMETRY;
		}
		if (endsWith(s, ".tesc")) {
			return GLSLANG_STAGE_TESSCONTROL;
		}
		if (endsWith(s, ".tese")) {
			return GLSLANG_STAGE_TESSEVALUATION;
		}
		if (endsWith(s, ".comp")) {
			return GLSLANG_STAGE_COMPUTE;
		}
		return GLSLANG_STAGE_VERTEX;
	}


	static bool CompileShader(VkDevice& device, glslang_stage_t Stage, const char* pShaderCode, coreShader& ShaderModule) {
		
		glslang_input_t input = {};
		input.language = GLSLANG_SOURCE_GLSL;
		input.stage = Stage;
		input.client = GLSLANG_CLIENT_VULKAN;
		input.client_version = GLSLANG_TARGET_VULKAN_1_1;
		input.target_language = GLSLANG_TARGET_SPV;
		input.target_language_version = GLSLANG_TARGET_SPV_1_3;
		input.code = pShaderCode;
		input.default_version = 100;
		input.default_profile = GLSLANG_NO_PROFILE;
		input.force_default_version_and_profile = false;
		input.forward_compatible = false;
		input.messages = GLSLANG_MSG_DEFAULT_BIT;
		input.resource = glslang_default_resource();

		glslang_shader_t* shader = glslang_shader_create(&input);

		if (!glslang_shader_preprocess(shader, &input)) {
			fprintf(stderr, "Couldn compile shader\n");
			fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
			fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
			//PrintShaderSource(input.code);
			return 0;
		}

		if (!glslang_shader_parse(shader, &input)) {
			fprintf(stderr, "Couldn parse shader\n");
			fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
			fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
			//PrintShaderSource(input.code);
			return 0;
		}

		glslang_program_t* program = glslang_program_create();
		glslang_program_add_shader(program, shader);

		if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT|GLSLANG_MSG_VULKAN_RULES_BIT)) {
			fprintf(stderr, "Couldn GLSL link\n");
			fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
			fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
			//PrintShaderSource(input.code);
			return 0;
		}


		glslang_program_SPIRV_generate(program, Stage);

		ShaderModule.Initialize(program);

		const char* spirv_messages = glslang_program_SPIRV_get_messages(program);

		if (spirv_messages) {
			fprintf(stderr, "Spir-v messages: '%s'", spirv_messages);
		}

		VkShaderModuleCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = ShaderModule.SPIRV.size() * sizeof(uint32_t);
		shaderCreateInfo.pCode = (const uint32_t*)ShaderModule.SPIRV.data();

		VkResult res = vkCreateShaderModule(device, &shaderCreateInfo, NULL, &ShaderModule.ShaderModule);
		CHECK_VK_RESULT(res, "vkCreateShaderModule\n");

		glslang_program_delete(program);
		glslang_shader_delete(shader);



		return ShaderModule.SPIRV.size() > 0;

	}

	void WriteBinaryFile(const char* filename, uint32_t* out, int size) {
		std::ofstream outfile(filename, std::ofstream::binary);
		outfile.write(reinterpret_cast<char*>(&size), sizeof(size));  // write it to the file
		outfile.write((const char*)out, size);                      // write the actual text
		outfile.close();
	}

	char* ReadBinaryFile(const char* fileName, int size) {
		std::ifstream inputFileStream;
		inputFileStream.open(fileName, std::ios::in | std::ios::binary);
		std::vector<char> fileContent;
		for (int i = 0; i < size; i++)
			inputFileStream.read((char*)&fileContent[i], sizeof(char));

		return fileContent.data();
	}

	VkShaderModule CreateShaderModuleFromText(VkDevice& device, const char* pFilename) {

		std::string Source;

		if (!Read_pFile(pFilename, Source)) {
			assert(0);
		}

		coreShader ShaderModule;

		glslang_stage_t ShaderStage = ShaderStageFromFilename(pFilename);

		VkShaderModule m = NULL;
		
		glslang_initialize_process();
		bool Success = CompileShader(device, ShaderStage, Source.c_str(), ShaderModule);

		if (Success) {
			printf("\nCreated shader from text file '%s'\n", pFilename);
			m = ShaderModule.ShaderModule;
			std::string BinaryFilename = std::string(pFilename) + ".spv";
			WriteBinaryFile(BinaryFilename.c_str(), ShaderModule.SPIRV.data(), (int)ShaderModule.SPIRV.size()*sizeof(uint32_t));
		}
		glslang_finalize_process();
		return m;
	}

	VkShaderModule CreateShaderModuleFromBinary(VkDevice& device, const char* pFilename) {
		int codeSize = 0;
		char* pShaderCode = ReadBinaryFile(pFilename, codeSize);
		assert(pShaderCode);

		VkShaderModuleCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = (size_t)codeSize;
		shaderCreateInfo.pCode = (const uint32_t*)pShaderCode;

		VkShaderModule shaderModule;
		VkResult res = vkCreateShaderModule(device, &shaderCreateInfo, NULL, &shaderModule);
		CHECK_VK_RESULT(res, "vkCreateShaderModule\n");
		printf("Created shader from binary %s\n", pFilename);

		free(pShaderCode);

		return shaderModule;
	}
}