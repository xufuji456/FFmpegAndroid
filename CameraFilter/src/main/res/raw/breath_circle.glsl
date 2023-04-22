precision mediump float;

varying highp vec2 textureCoordinate;

uniform sampler2D inputImageTexture;
uniform vec2  uCenter;
uniform float uInnerRadius;
uniform float uOuterRadius;

void main() {
	vec3 textureColor = texture2D(inputImageTexture, textureCoordinate).rgb;

	float dist = distance(textureCoordinate, uCenter);
	float scale = clamp(1.0 - (dist - uInnerRadius) / (uOuterRadius - uInnerRadius), 0.0, 1.0);

	gl_FragColor = vec4(textureColor.r * scale, textureColor.g * scale, textureColor.b * scale, 1.0);
}