precision mediump float;

varying vec2 textureCoordinate;

uniform sampler2D inputImageTexture;
uniform float brightness;

void main() {
    vec3 textureColor = texture2D(inputImageTexture, textureCoordinate).rgb;

    gl_FragColor = vec4(textureColor.rgb + vec3(brightness), 1.0);
}