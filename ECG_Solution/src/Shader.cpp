#include "Shader.h"
/*
GLuint Shader::loadShaders()
{
    // shader object
    GLuint vertexHandle, fragmentHandle;

    if (loadShader(_vs, GL_VERTEX_SHADER, vertexHandle) == GL_TRUE &&
        loadShader(_fs, GL_FRAGMENT_SHADER, fragmentHandle) == GL_TRUE)
    {
        // vertex + fragment shader successfully compiled
        // program Object
        // Generate Object
        _handle = glCreateProgram();
        // Attach Shader Objects
        glAttachShader(_handle, vertexHandle);
        glAttachShader(_handle, fragmentHandle);
        // Link Program
        glLinkProgram(_handle);

        // Check errors
        GLint succeded;
        glGetProgramiv(_handle, GL_LINK_STATUS, &succeded);
        if (!succeded) {
           
            // Log auslesen und ausgeben
            GLint logSize;
            glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &logSize);

            GLchar* message = new char[logSize];
            glGetProgramInfoLog(_handle, logSize, nullptr, message);

            std::cout << "error from me: " << message;

            //shader not needed anymore
            glDeleteProgram(_handle);
            glDeleteShader(fragmentHandle);
            glDeleteShader(vertexHandle);

            delete[] message;
        }
    }
    return _handle;
}

bool Shader::loadShader(string file, GLenum shaderType, GLuint& handle)
{
    // generate shader object
    handle = glCreateShader(shaderType);

    // Load Source Code; Send the shader source code to GL
    std::ifstream shaderFile(file);
    string fileS((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

    const char* sourceS = (const GLchar*)fileS.c_str();
    glShaderSource(handle, 1, &sourceS, 0);

    // Compile the shader
    glCompileShader(handle);
    // check for errors
    GLint succededShader;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &succededShader);
    if (succededShader == GL_FALSE) {
        // Log auslesen und ausgeben
        GLint logSize;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logSize);

        GLchar* message = new char[logSize];
        glGetShaderInfoLog(handle, logSize, nullptr, message);
        std::cout << "error from me: " << message;

        //shader not needed anymore
        glDeleteShader(handle);

        delete[] message;
    }
    return succededShader;
}

GLint Shader::getUniformLocation(string uniform)
{
    if (_locations.find(uniform) == _locations.end())
        _locations[uniform] = glGetUniformLocation(_handle, uniform.c_str());

    return _locations[uniform];
}

/*!
* Default constructor of a simple color shader
*//*
Shader::Shader() {
}

Shader::Shader(string vs, string fs) : _vs(vs), _fs(fs)
{
    loadShaders();
    use();
}

Shader::~Shader()
{
    glDeleteProgram(_handle);
}

void Shader::use() const
{
    glUseProgram(_handle);
}

void Shader::unuse() const
{
    glUseProgram(0);
}

void Shader::setUniform(string uniform, const int i)
{
    glUniform1i(getUniformLocation(uniform), i);
}

void Shader::setUniform(GLint location, const int i)
{
    glUniform1i(location, i);
}

void Shader::setUniform(string uniform, const unsigned int i)
{
    glUniform1ui(getUniformLocation(uniform), i);
}

void Shader::setUniform(GLint location, const unsigned int i)
{
    glUniform1ui(location, i);
}

void Shader::setUniform(string uniform, const float f)
{
    glUniform1f(getUniformLocation(uniform), f);
}

void Shader::setUniform(GLint location, const float f)
{
    glUniform1f(location, f);
}

void Shader::setUniform(string uniform, const glm::mat4& mat)
{
    glUniformMatrix4fv(getUniformLocation(uniform), 1, false, glm::value_ptr(mat));
}

void Shader::setUniform(GLint location, const glm::mat4& mat)
{
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(mat));
}

void Shader::setUniform(string uniform, const glm::mat3& mat)
{
    glUniformMatrix3fv(getUniformLocation(uniform), 1, false, glm::value_ptr(mat));
}

void Shader::setUniform(GLint location, const glm::mat3& mat)
{
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(mat));
}

void Shader::setUniform(string uniform, const glm::vec2& vec)
{
    glUniform2fv(getUniformLocation(uniform), 1, glm::value_ptr(vec));
}

void Shader::setUniform(GLint location, const glm::vec2& vec)
{
    glUniform2fv(location, 1, glm::value_ptr(vec));
}

void Shader::setUniform(string uniform, const glm::vec3& vec)
{
    glUniform3fv(getUniformLocation(uniform), 1, glm::value_ptr(vec));
}

void Shader::setUniform(GLint location, const glm::vec3& vec)
{
    glUniform3fv(location, 1, glm::value_ptr(vec));
}

void Shader::setUniform(string uniform, const glm::vec4& vec)
{
    glUniform4fv(getUniformLocation(uniform), 1, glm::value_ptr(vec));
}

void Shader::setUniform(GLint location, const glm::vec4& vec)
{
    glUniform4fv(location, 1, glm::value_ptr(vec));
}


void Shader::setUniformArr(string arr, unsigned int i, string prop, const glm::vec3& vec)
{
    glUniform3fv(getUniformLocation(arr + "[" + std::to_string(i) + "]." + prop), 1, glm::value_ptr(vec));
}

void Shader::setUniformArr(string arr, unsigned int i, string prop, const float f)
{
    glUniform1f(getUniformLocation(arr + "[" + std::to_string(i) + "]." + prop), f);
}
*/