#ifndef SHADER_FACTORY_H
#define SHADER_FACTORY_H

#include <string>

std::string GetVertexShader();
std::string GetFragmentShader();
std::string GetVertexShaderString(int textureNum);

std::string GetYUV420p2RGBFragmentShader(bool fullRange);
std::string GetYUV420sp2RGBFragmentShader(bool fullRange);
std::string GetYUV420p10le2RGBFragmentShader(bool fullRange);

#endif // SHADER_FACTORY_H
