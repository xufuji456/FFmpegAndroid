varying highp vec2 textureCoordinate;

uniform sampler2D inputImageTexture;

void main()
{
	lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);

	gl_FragColor = vec4((1.0 - textureColor.rgb), 1.0);
}