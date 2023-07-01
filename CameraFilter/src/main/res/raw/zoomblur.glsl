varying highp vec2 textureCoordinate;

uniform sampler2D inputImageTexture;

uniform highp vec2 blurCenter;
uniform highp float blurSize;

void main()
{

	highp vec2 samplingOffset = 1.0/100.0 * (blurCenter - textureCoordinate) * blurSize;

	lowp vec4 blurColor = texture2D(inputImageTexture, textureCoordinate) * 0.18;
	blurColor += texture2D(inputImageTexture, textureCoordinate + samplingOffset) * 0.15;
	blurColor += texture2D(inputImageTexture, textureCoordinate + (2.0 * samplingOffset)) * 0.12;
	blurColor += texture2D(inputImageTexture, textureCoordinate + (3.0 * samplingOffset)) * 0.09;
	blurColor += texture2D(inputImageTexture, textureCoordinate + (4.0 * samplingOffset)) * 0.05;
	blurColor += texture2D(inputImageTexture, textureCoordinate - samplingOffset) * 0.15;
	blurColor += texture2D(inputImageTexture, textureCoordinate - (2.0 * samplingOffset)) * 0.12;
	blurColor += texture2D(inputImageTexture, textureCoordinate - (3.0 * samplingOffset)) * 0.09;
	blurColor += texture2D(inputImageTexture, textureCoordinate - (4.0 * samplingOffset)) * 0.05;

	gl_FragColor = blurColor;
}