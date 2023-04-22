precision mediump float;

varying vec2 textureCoordinate;

uniform sampler2D inputImageTexture;
uniform float saturation;

const vec3 lumaWeight = vec3(0.2125, 0.7154, 0.0721);

void main() {
    vec3 textureColor = texture2D(inputImageTexture, textureCoordinate).rgb;
    float luma = dot(textureColor, lumaWeight);
    vec3 lumaColor = vec3(luma);
    vec3 outputColor = mix(lumaColor, textureColor, saturation);
    gl_FragColor = vec4(outputColor, 1.0);
}