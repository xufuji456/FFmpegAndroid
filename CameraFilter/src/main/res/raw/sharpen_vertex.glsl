precision mediump float;

attribute vec4 position;
attribute vec4 inputTextureCoordinate;

varying vec2 textureCoordinate;
varying vec2 leftTextureCoordinate;
varying vec2 rightTextureCoordinate;
varying vec2 topTextureCoordinate;
varying vec2 bottomTextureCoordinate;

varying float centerMultiplier;
varying float edgeMultiplier;
uniform float imageWidthFactor;
uniform float imageHeightFactor;
uniform float sharpness;

void main() {
    gl_Position = position;
    vec2 width  = vec2(imageWidthFactor, 0.0);
    vec2 height = vec2(0.0, imageHeightFactor);

    textureCoordinate       = inputTextureCoordinate.xy;
    leftTextureCoordinate   = inputTextureCoordinate.xy - width;
    rightTextureCoordinate  = inputTextureCoordinate.xy + width;
    topTextureCoordinate    = inputTextureCoordinate.xy + height;
    bottomTextureCoordinate = inputTextureCoordinate.xy - height;

    centerMultiplier = 1.0 + 4.0 * sharpness;
    edgeMultiplier   = sharpness;
}