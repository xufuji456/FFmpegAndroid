/**
 * Note: shader factory of vertex and fragment
 *       1. YUV420P
 *       2. YUV420SP
 *       3. YUV420P10LE
 *
 * Date: 2025/12/21
 * Author: frank
 */

#include "ShaderFactory.h"

#include <sstream>

#define SHADER_STRING(text) #text

static const std::string CommonVertexShader =
    SHADER_STRING(
        precision highp float;
        attribute vec4 position;
        attribute vec4 texCoord;
        varying vec2 vTexCoord;
        uniform mat4 uMvp;

        void main() {
            gl_Position = uMvp * position;
            vTexCoord = texCoord.xy;
        }
    );

static const std::string CommonFragmentShader =
    SHADER_STRING(
        precision highp float;
        varying vec2 vTexCoord;
        uniform sampler2D colorMap;

        void main() {
            gl_FragColor = vec4(texture2D(colorMap, vTexCoord).rgb, 1.0);
        }
    );

static const std::string YUV420p2RGBFragmentShader =
    SHADER_STRING(
        precision highp float;
        varying vec2 vTexCoord;
        varying vec2 vTexCoord1;
        varying vec2 vTexCoord2;
        uniform mat3 uColorConversion;
        uniform sampler2D colorMap;
        uniform sampler2D colorMap1;
        uniform sampler2D colorMap2;

        void main() {
            vec3 yuv;
            vec3 rgb;

            yuv.x = (texture2D(colorMap,  vTexCoord).r  - (16.0 / 255.0));
            yuv.y = (texture2D(colorMap1, vTexCoord1).r - 0.5);
            yuv.z = (texture2D(colorMap2, vTexCoord2).r - 0.5);
            rgb   = uColorConversion * yuv;
            gl_FragColor = vec4(rgb, 1);
        }
    );

static const std::string YUV420sp2RGBFragmentShader =
    SHADER_STRING(
        precision highp float;
        varying vec2 vTexCoord;
        varying vec2 vTexCoord1;
        uniform mat3 uColorConversion;
        uniform sampler2D colorMap;
        uniform sampler2D colorMap1;

        void main() {
           vec3 yuv;
           vec3 rgb;

           yuv.x  = (texture2D(colorMap,  vTexCoord).r   - (16.0 / 255.0));
           yuv.yz = (texture2D(colorMap1, vTexCoord1).rg - vec2(0.5, 0.5));
           rgb    = uColorConversion * yuv;
           gl_FragColor = vec4(rgb, 1);
        }
    );

static const std::string YUV420p10le2RGBFragmentShader =
    SHADER_STRING(
        precision highp float;
        varying vec2 vTexCoord;
        varying vec2 vTexCoord1;
        varying vec2 vTexCoord2;
        uniform mat3 uColorConversion;
        uniform sampler2D colorMap;
        uniform sampler2D colorMap1;
        uniform sampler2D colorMap2;

        void main() {
           vec3 yuv_l;
           vec3 yuv_h;
           vec3 yuv;
           vec3 rgb;

           yuv_l.x = texture2D(colorMap,  vTexCoord).r;
           yuv_h.x = texture2D(colorMap,  vTexCoord).a;
           yuv_l.y = texture2D(colorMap1, vTexCoord1).r;
           yuv_h.y = texture2D(colorMap1, vTexCoord1).a;
           yuv_l.z = texture2D(colorMap2, vTexCoord2).r;
           yuv_h.z = texture2D(colorMap2, vTexCoord2).a;

           yuv = (yuv_l * 255.0 + yuv_h * 255.0 * 256.0) / (1023.0) - vec3(16.0 / 255.0, 0.5, 0.5);

           rgb = uColorConversion * yuv;
           gl_FragColor = vec4(rgb, 1);
        }
    );

std::string GetVertexShader() {
    return CommonVertexShader;
}

std::string GetFragmentShader() {
    return CommonFragmentShader;
}

std::string GetVertexShaderString(int textureNum)  {
    if (textureNum <= 1) {
        return GetVertexShader();
    }

    std::string shaderStr = "\
            precision highp float;\n\
            attribute vec4 position;\n\
            attribute vec4 texCoord;\n\
            varying vec2 vTexCoord;\n\
            uniform mat4 uMvp;\n\
            ";
    for (int i = 1; i < textureNum; ++i) {
        std::stringstream oss;
        oss << "attribute vec4 texCoord" << i << ";\n"
            << "varying vec2 vTexCoord" << i << ";\n";
        shaderStr += oss.str();
    }
    shaderStr += "void main() {\n";
    shaderStr += "gl_Position = uMvp * position;\n";
    shaderStr += "vTexCoord = texCoord.xy;\n";
    for (int i = 1; i < textureNum; ++i) {
        std::stringstream oss;
        oss << "vTexCoord" << i << " = texCoord" << i << ".xy;\n";
        shaderStr += oss.str();
    }
    shaderStr += "}\n";

    return shaderStr;
}

std::string HandleColorRange(const std::string &src, bool fullRange) {
    if (fullRange) {
        std::string shader = src;
        size_t pos = shader.find("16.0");
        if (pos != std::string::npos) {
            shader.replace(pos, 4, "0.0");
        }
        return shader;
    }
    return src;
}

std::string GetYUV420p2RGBFragmentShader(bool fullRange) {
    return HandleColorRange(YUV420p2RGBFragmentShader, fullRange);
}

std::string GetYUV420sp2RGBFragmentShader(bool fullRange) {
    return HandleColorRange(YUV420sp2RGBFragmentShader, fullRange);
}

std::string GetYUV420p10le2RGBFragmentShader(bool fullRange) {
    return HandleColorRange(YUV420p10le2RGBFragmentShader, fullRange);
}
