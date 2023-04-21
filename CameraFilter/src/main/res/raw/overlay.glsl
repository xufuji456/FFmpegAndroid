precision mediump float;

varying highp vec2 textureCoordinate;

uniform sampler2D inputImageTexture;
uniform sampler2D overlayTexture;

void main()
{
	vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
	vec4 overlayColor = texture2D(overlayTexture, textureCoordinate);

	gl_FragColor = mix(textureColor, overlayColor, overlayColor.a);
}