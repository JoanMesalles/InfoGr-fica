#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include "GL_framework.h"
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include "GL_framework.h"

class Shader {
public:
	unsigned int programID;

	Shader(const char* vertexPath, const char* fragmentPath);
	Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

	void Use();
	void StopUse();
	void SetUp();
	void SetBool(const std::string& name, bool value);
	void SetInt(const std::string& name, int value);
	void SetFloat(const std::string& name, float value);
	void SetMatrix(const std::string& name, glm::mat4 value);
	void SetVector(const std::string& name, glm::vec4 value);
	void CleanUp();
private:
	int type;
	std::string vShaderCode, fShaderCode, gShaderCode;
	GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name);
	void linkProgram(GLuint program);

};