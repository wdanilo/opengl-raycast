#pragma once
#include <vector>
#include <glew.h>


GLuint LoadProgram(const char* vsKey, const char* gsKey, const char* fsKey);
void SetUniform(const char* name, int value);
void SetUniform(const char* name, float value);
void SetUniform(const char* name, float x, float y);
void SetUniform(const char* name, vmath::Matrix4 value);
void SetUniform(const char* name, vmath::Matrix3 value);
void SetUniform(const char* name, vmath::Vector3 value);
void SetUniform(const char* name, vmath::Point3 value);
void SetUniform(const char* name, vmath::Vector4 value);

