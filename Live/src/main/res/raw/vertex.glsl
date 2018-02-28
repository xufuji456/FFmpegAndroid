attribute vec4 position;
attribute vec4 inputTextureCoordinate;

varying vec2 textureCoordinate;

uniform mat4 textureTransform;

void main() {
    textureCoordinate = (textureTransform * inputTextureCoordinate).xy;
    gl_Position = position;
}
