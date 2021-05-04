//#include "Shader.h"
#include <cstdio>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <glm/glm.hpp>
#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include "GL_framework.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
	void setupAxis();
	void cleanupAxis();
	void drawAxis();
}
////////////////

namespace RenderVars {
	float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 150.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

extern bool loadOBJ(const char* path, std::vector < glm::vec3 >& out_vertices, std::vector < glm::vec2 >& out_uvs, std::vector < glm::vec3 >& out_normals);

float w, h;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	w = width;
	h = height;
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	}
	else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

//////////////////////////////////////////////////

GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char* buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char* buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}


////////////////////////////////////////////////// AXIS
namespace Axis {
	GLuint AxisVao;
	GLuint AxisVbo[3];
	GLuint AxisShader[2];
	GLuint AxisProgram;

	float AxisVerts[] = {
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0
	};
	float AxisColors[] = {
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};
	GLubyte AxisIdx[] = {
		0, 1,
		2, 3,
		4, 5
	};
	const char* Axis_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec4 in_Color;\n\
out vec4 vert_color;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	vert_color = in_Color;\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
	const char* Axis_fragShader =
		"#version 330\n\
in vec4 vert_color;\n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = vert_color;\n\
}";

	void setupAxis() {
		glGenVertexArrays(1, &AxisVao);
		glBindVertexArray(AxisVao);
		glGenBuffers(3, AxisVbo);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

		AxisProgram = glCreateProgram();
		glAttachShader(AxisProgram, AxisShader[0]);
		glAttachShader(AxisProgram, AxisShader[1]);
		glBindAttribLocation(AxisProgram, 0, "in_Position");
		glBindAttribLocation(AxisProgram, 1, "in_Color");
		linkProgram(AxisProgram);
	}
	void cleanupAxis() {
		glDeleteBuffers(3, AxisVbo);
		glDeleteVertexArrays(1, &AxisVao);

		glDeleteProgram(AxisProgram);
		glDeleteShader(AxisShader[0]);
		glDeleteShader(AxisShader[1]);
	}
	void drawAxis() {
		glBindVertexArray(AxisVao);
		glUseProgram(AxisProgram);
		glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

//Parameters
//This are the parameters to be changed for the lighting effects
glm::vec4 cameraPos = glm::vec4(0, 0, 0, 1);
glm::vec4 lightPos = glm::vec4(0, 0, 0, 1);
glm::vec4 pointingLight = glm::vec4(0, 0, 0, 1);
glm::vec4 ligthDir = glm::vec4(-20.0f, 0, 0, 1.0f);
glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
glm::mat4 attempt = glm::mat4(1);
float mode = 0;
float lightIntesity = 10.0f;
float fadeAmount = 3.0f;
float angle = 30.f;
float ambientI = 0.5f;
float diffuseI = 0.5f;
glm::vec4 ambientColor = glm::vec4(1, 1, 1, 0);
double timer;
bool explosion = false;
int ex = 1;

//Dolly Effect
//Variables needed to do the dolly effect
bool ZoomEffect = false;
float initZ;
float angelFOV = 65.0f;

float PI = 3.14159265359;

class Shader {
	int type;
	GLuint textureID;
	std::string vShaderCode, fShaderCode, gShaderCode;
	GLenum format;
public:
	unsigned int programID;
	Shader() {

	}
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		type = 0;
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		vShaderCode = vertexCode.c_str();
		fShaderCode = fragmentCode.c_str();
	}

	Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
	{
		type = 1;
		std::string vertexCode;
		std::string fragmentCode;
		std::string geomtryCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		std::ifstream gShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			gShaderFile.open(geometryPath);
			std::stringstream vShaderStream, fShaderStream, gShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			gShaderStream << gShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			gShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
			geomtryCode = gShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		vShaderCode = vertexCode.c_str();
		fShaderCode = fragmentCode.c_str();
		gShaderCode = geomtryCode.c_str();
	}

	void Use()
	{
		glUseProgram(programID);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void StopUse()
	{
		glDisable(GL_BLEND);
		glUseProgram(0);
	}

	void SetUp()
	{
		if (type == 0) {
			GLuint vertex, fragment;
			vertex = compileShader(vShaderCode.c_str(), GL_VERTEX_SHADER, "vertexShader");
			fragment = compileShader(fShaderCode.c_str(), GL_FRAGMENT_SHADER, "fragmentShader");
			programID = glCreateProgram();
			glAttachShader(programID, vertex);
			glAttachShader(programID, fragment);
			glBindAttribLocation(programID, 0, "in_Position");
			glBindAttribLocation(programID, 1, "in_Normal");
			glBindAttribLocation(programID, 2, "in_UV");
			linkProgram(programID);

			glDeleteShader(vertex);
			glDeleteShader(fragment);
		}
		else {

			GLuint vertex, fragment, geometry;
			vertex = compileShader(vShaderCode.c_str(), GL_VERTEX_SHADER, "vertexShader");
			fragment = compileShader(fShaderCode.c_str(), GL_FRAGMENT_SHADER, "fragmentShader");
			geometry = compileShader(gShaderCode.c_str(), GL_GEOMETRY_SHADER, "geometryShader");

			programID = glCreateProgram();
			glAttachShader(programID, vertex);
			glAttachShader(programID, fragment);
			glAttachShader(programID, geometry);
			glBindAttribLocation(programID, 0, "in_Position");
			glBindAttribLocation(programID, 1, "in_Normal");
			glBindAttribLocation(programID, 2, "in_UV");
			linkProgram(programID);

			glDeleteShader(vertex);
			glDeleteShader(fragment);
			glDeleteShader(geometry);
		}

	}

	void SetBool(const std::string& name, bool value)
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
	}

	void SetInt(const std::string& name, int value)
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
	}

	void SetFloat(const std::string& name, float value)
	{
		glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
	}

	void SetMatrix(const std::string& name, glm::mat4 value)
	{
		glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}

	void SetVector(const std::string& name, glm::vec4 value)
	{
		glUniform4f(glGetUniformLocation(programID, name.c_str()), value.x, value.y, value.z, value.w);
	}

	void SetTexture(const char* texturePath)
	{
		//Texture
		glGenTextures(1, &textureID); // Create the handle of the texture
		int x, y, n;
		unsigned char* data = stbi_load(texturePath, &x, &y, &n, 0);
		if (data)
		{
			if (n == 1)
				format = GL_RED;
			else if (n == 3)
				format = GL_RGB;
			else if (n == 4)
				format = GL_RGBA;
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			stbi_image_free(data);
			glDeleteTextures(0, &textureID);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}

	}

	void ActivateTexture()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(programID, "Texture"), 0);
	}

	void CleanUp()
	{
	}

	GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name)
	{
		GLuint shaderp = glCreateShader(shaderType);
		glShaderSource(shaderp, 1, &shaderStr, NULL);
		glCompileShader(shaderp);
		GLint res;
		glGetShaderiv(shaderp, GL_COMPILE_STATUS, &res);
		if (res == GL_FALSE) {
			glGetShaderiv(shaderp, GL_INFO_LOG_LENGTH, &res);
			char* buff = new char[res];
			glGetShaderInfoLog(shaderp, res, &res, buff);
			fprintf(stderr, "Error Shader %s: %s", name, buff);
			delete[] buff;
			glDeleteShader(shaderp);
			return 0;
		}
		return shaderp;
	}

	void linkProgram(GLuint program)
	{
		glLinkProgram(program);
		GLint res;
		glGetProgramiv(program, GL_LINK_STATUS, &res);
		if (res == GL_FALSE) {
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
			char* buff = new char[res];
			glGetProgramInfoLog(program, res, &res, buff);
			fprintf(stderr, "Error Link: %s", buff);
			delete[] buff;
		}
	}
};


////////// Model class
class Model {
	GLuint modelVao;
	GLuint modelVbo[3];
	Shader* shader;
	glm::mat4 objMat;
	glm::mat4 normalMat;
	std::vector< glm::vec3 > verticesModel;
	std::vector< glm::vec2 > uvsModel;
	std::vector< glm::vec3 > normalsModel;
	char* path;
	char* texturePath;
public:
	Model(char* _path, glm::mat4 _mat, Shader* _shader, char* _texturePath) {
		path = _path;
		objMat = _mat;
		normalMat = glm::transpose(glm::inverse(_mat));
		shader = _shader;
		texturePath = _texturePath;
	}

	void setupModel() {
		bool res = loadOBJ(path, verticesModel, uvsModel, normalsModel);

		glGenVertexArrays(1, &modelVao);
		glBindVertexArray(modelVao);
		glGenBuffers(3, modelVbo);

		glBindBuffer(GL_ARRAY_BUFFER, modelVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, verticesModel.size() * sizeof(glm::vec3), &verticesModel[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, modelVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normalsModel.size() * sizeof(glm::vec3), &normalsModel[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, modelVbo[2]);
		glBufferData(GL_ARRAY_BUFFER, uvsModel.size() * sizeof(glm::vec2), &uvsModel[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		shader->SetTexture(texturePath);


	}
	void cleanupModel() {
		glDeleteBuffers(3, modelVbo);
		glDeleteVertexArrays(1, &modelVao);
	}
	void updateModel(glm::mat4 transform) {
		objMat = transform;
	}
	void drawModel(Shader* shader, double currentTime) {
		glBindVertexArray(modelVao);
		shader->Use();
		shader->ActivateTexture();
		shader->SetMatrix("objMat", objMat);
		shader->SetMatrix("mv_Mat", RenderVars::_modelView);
		shader->SetMatrix("mvpMat", RenderVars::_MVP);
		shader->SetMatrix("nv_Mat", normalMat);
		shader->SetMatrix("proj_Mat", RV::_projection);
		shader->SetVector("ambientColor", ambientColor);
		shader->SetFloat("ambientColorI", ambientI);
		shader->SetVector("lightColor", lightColor);
		shader->SetVector("lightDir", ligthDir);
		shader->SetVector("lightPos", lightPos);
		shader->SetVector("CameraPos", cameraPos);
		shader->SetVector("lightPointing", pointingLight);
		shader->SetFloat("diffuseIntensity", diffuseI);
		shader->SetFloat("outterCuttOff", glm::cos(glm::radians(-angle - fadeAmount)));
		shader->SetFloat("cuttOff", glm::cos(glm::radians(angle)));
		shader->SetFloat("lightIntensity", lightIntesity);
		shader->SetFloat("time", currentTime);
		shader->SetFloat("mode", mode);
		shader->SetVector("color", color);
		shader->SetBool("explosion", explosion);

		glDrawArrays(GL_TRIANGLES, 0, verticesModel.size());
		shader->StopUse();
		glBindVertexArray(0);
	}
};

namespace Billboard {
	GLuint BillboardVao;
	GLuint BillboardVbo[2];
	Shader BillboardShader("res/vShaderBillboard.txt","res/gShaderBillboard.txt" ,"res/fShaderBillboard.txt");
	float BillboardVerts[] = {
	0.0, 0.0, 0.0
	};
	GLubyte BillboardIdx[] = {
	0
	};

	void setupBillboard() {
		glGenVertexArrays(1, &BillboardVao);
		glBindVertexArray(BillboardVao);
		glGenBuffers(2, BillboardVbo);

		glBindBuffer(GL_ARRAY_BUFFER, BillboardVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3, BillboardVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BillboardVbo[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 1, BillboardIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		BillboardShader.SetUp();
		BillboardShader.SetTexture("res/tree.png");

	}
	void cleanupBillboard() {
		glDeleteBuffers(2, BillboardVbo);
		glDeleteVertexArrays(1, &BillboardVao);
		BillboardShader.CleanUp();

	}
	void drawBillboard() {
		glBindVertexArray(BillboardVao);
		BillboardShader.Use();
		BillboardShader.ActivateTexture();
		BillboardShader.SetMatrix("mvpMat", RenderVars::_MVP);
		glDrawArrays(GL_POINTS, 0, 4);

		BillboardShader.StopUse();
		glBindVertexArray(0);
	}

}

Shader phongShader("res/vShader.txt", "res/gShader.txt", "res/fShader.txt");

Model car("res/car.obj", glm::scale(glm::mat4(), glm::vec3(2.0f, 2.0f, 2.0f)), &phongShader, "res/metal.png");


void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// Setup shaders & geometry
	phongShader.SetUp();
	//palmShader.SetUp();
	Axis::setupAxis();
	Billboard::setupBillboard();
	car.setupModel();

	w = width;
	h = height;
}

void GLcleanup() {
	Axis::cleanupAxis();
	Billboard::cleanupBillboard();
	car.cleanupModel();
	phongShader.CleanUp();
}

void GLrender(float dt) {
	if (explosion == false) {
		timer = 0;
	}
	else {
		timer += dt;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RV::_projection = glm::perspective(RV::FOV, (float)w / (float)h, RV::zNear, RV::zFar);
	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	cameraPos = glm::inverse(RV::_modelView) * glm::vec4(0,0,0,1);
	Axis::drawAxis();
	switch (ex)
	{
	case 0:
		Billboard::drawBillboard();
		break;
	case 1:
		car.drawModel(&phongShader, timer);
		break;
	default:
		break;
	}

	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SliderInt("Exercice", &ex, 0, 1);
		ImGui::Checkbox("Explosion", &explosion);
		if (ex == 1) {
			if (ImGui::CollapsingHeader("Phong")) {
				if (ImGui::Button("Direccional Light"))
				{
					mode = 0;
				}
				if (ImGui::Button("Point Light"))
				{

					mode = 1;
				}
				if (ImGui::Button("Spotlight"))
				{
					mode = 2;
					lightPos.x = -5.0f;
					lightPos.y = 5.0f;
					lightPos.z = 3.0f;
					pointingLight.x = 4.7f;
					pointingLight.y = -2.9f;
					pointingLight.z = -3.0f;
					lightIntesity = 130.0f;

				}
				if (mode == 0) {
					ImGui::SliderFloat3("Light Pos", &ligthDir.x, -20, +20);
					ImGui::SliderFloat("Ambient Intensity", &ambientI, +0, +1);
					ImGui::SliderFloat("Difuse Intensity", &diffuseI, +0, +1);
					ImGui::ColorEdit3("Ambient Colour", &ambientColor.x);
				}
				if (mode == 1) {
					ImGui::SliderFloat3("Light Pos", &lightPos.x, -20, +20);
					ImGui::SliderFloat("Light Intensity", &lightIntesity, +1, +1000);
					ImGui::ColorEdit3("Ambient Colour", &ambientColor.x);
				}
				if (mode == 2) {
					ImGui::SliderFloat3("Light Pos", &lightPos.x, -20, +20);
					ImGui::SliderFloat3("Light Pointing", &pointingLight.x, -20, +20);
					ImGui::SliderFloat("Light Intensity", &lightIntesity, +1, +1000);
					ImGui::SliderFloat("Angle", &angle, +1, +180);
					ImGui::SliderFloat("Fade Amount", &fadeAmount, +1, +30);
					ImGui::ColorEdit3("Ambient Colour", &ambientColor.x);
				}
			}

		}

		//Inicialize Dolly Effect
		//if (ImGui::Button("Inversed Dolly effect"))
		//{
		//	//Reset camera Position and initial Fov
		//	RV::panv[0] = { 0.f }; RV::panv[1] = { -3.f }; RV::panv[2] = { -15.f };
		//	RV::rota[0] = { 0.f }; RV::rota[1] = { 0.f };
		//	angelFOV = 20.0f;

		//	if (ZoomEffect == false) { ZoomEffect = true; }
		//	else if (ZoomEffect == true) 
		//	{ 
		//		ZoomEffect = false;
		//		RV::panv[0] = { 0.f }; RV::panv[1] = { -5.f }; RV::panv[2] = { -15.f };
		//		RV::rota[0] = { 0.f }; RV::rota[1] = { 0.f };
		//		RV::FOV = glm::radians(65.0f);
		//	}
		//}


		//if (ZoomEffect == true) {

		//	//Increase the distance 
		//	RV::panv[2] -= 0.5f;
		//	//Calculate the fov acording to the distance
		//	RV::FOV = 2 * atan2(10.0f, 2 * abs(RV::panv[2]));

		//	//Finish the animation when the camera gets too far
		//	if (RV::panv[2] < -90.0f)
		//	{
		//		ZoomEffect = false;
		//		RV::panv[0] = { 0.f }; RV::panv[1] = { -5.f }; RV::panv[2] = { -15.f };
		//		RV::rota[0] = { 0.f }; RV::rota[1] = { 0.f };
		//		RV::FOV = glm::radians(65.0f);
		//	}

		//}
	}

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}