#include "Shader.h"


Shader::Shader(const char* vertexPath, const char* fragmentPath)
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

    //vertex = compileShader(vShaderCode, GL_VERTEX_SHADER, "vertexShader");
    //fragment = compileShader(fShaderCode, GL_FRAGMENT_SHADER, "fragmentShader");


}

Shader::Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
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

void Shader::Use()
{
    glUseProgram(programID);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Shader::StopUse()
{
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void Shader::SetUp()
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

void Shader::SetBool(const std::string& name, bool value)
{
    glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::SetMatrix(const std::string& name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVector(const std::string& name, glm::vec4 value)
{
    glUniform4f(glGetUniformLocation(programID, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::CleanUp()
{
}

GLuint Shader::compileShader(const char* shaderStr, GLenum shaderType, const char* name)
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

void Shader::linkProgram(GLuint program)
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
