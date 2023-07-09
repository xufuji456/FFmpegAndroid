precision mediump float;

const int GAUSSIAN_SAMPLES = 9;

varying vec2 textureCoordinate;
varying vec2 blurCoordinates[GAUSSIAN_SAMPLES];

uniform vec2 blurCenter;
uniform float blurRadius;
uniform float aspectRatio;
uniform sampler2D inputImageTexture;

void main() {
	vec2 textureCoordinateToUse = vec2(textureCoordinate.x, (textureCoordinate.y * aspectRatio + 0.5 - 0.5 * aspectRatio));
	float dist = distance(blurCenter, textureCoordinateToUse);

	if (dist < blurRadius) {
		vec4 sum = vec4(0.0);

		sum += texture2D(inputImageTexture, blurCoordinates[0]) * 0.05;
		sum += texture2D(inputImageTexture, blurCoordinates[1]) * 0.09;
		sum += texture2D(inputImageTexture, blurCoordinates[2]) * 0.12;
		sum += texture2D(inputImageTexture, blurCoordinates[3]) * 0.15;
		sum += texture2D(inputImageTexture, blurCoordinates[4]) * 0.18;
		sum += texture2D(inputImageTexture, blurCoordinates[5]) * 0.15;
		sum += texture2D(inputImageTexture, blurCoordinates[6]) * 0.12;
		sum += texture2D(inputImageTexture, blurCoordinates[7]) * 0.09;
		sum += texture2D(inputImageTexture, blurCoordinates[8]) * 0.05;

		gl_FragColor = sum;
	} else {
		gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
	}
}