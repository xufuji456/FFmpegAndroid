attribute vec4 position;
attribute vec4 inputTextureCoordinate;

uniform mat4 textureTransform;
varying vec2 textureCoordinate;

void main()
{
	textureCoordinate = (textureTransform * inputTextureCoordinate).xy;
	gl_Position = position;
}
