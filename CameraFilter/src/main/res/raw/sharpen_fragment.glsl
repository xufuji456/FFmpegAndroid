precision mediump float;

varying vec2 textureCoordinate;
varying vec2 leftTextureCoordinate;
varying vec2 rightTextureCoordinate;
varying vec2 topTextureCoordinate;
varying vec2 bottomTextureCoordinate;
varying float centerMultiplier;
varying float edgeMultiplier;

uniform sampler2D inputImageTexture;

void main() {
    vec3 textureColor       = texture2D(inputImageTexture, textureCoordinate).rgb;
    vec3 leftTextureColor   = texture2D(inputImageTexture, leftTextureCoordinate).rgb;
    vec3 rightTextureColor  = texture2D(inputImageTexture, rightTextureCoordinate).rgb;
    vec3 topTextureColor    = texture2D(inputImageTexture, topTextureCoordinate).rgb;
    vec3 bottomTextureColor = texture2D(inputImageTexture, bottomTextureCoordinate).rgb;

    gl_FragColor = vec4((textureColor * centerMultiplier -
    ((leftTextureColor+rightTextureColor+topTextureColor+bottomTextureColor)* edgeMultiplier)),
    texture2D(inputImageTexture, bottomTextureCoordinate).w);
}