precision mediump float;

varying vec2 textureCoordinate;

uniform sampler2D inputImageTexture;
uniform float contrast;

const vec3 halfColor = vec3(0.5);

void main() {
    vec3 textureColor = texture2D(inputImageTexture, textureCoordinate).rgb;
    vec3 outputColor  = (textureColor - halfColor) * contrast + halfColor;
    gl_FragColor = vec4(outputColor, 1.0);
}