precision highp float;

varying highp vec2 textureCoordinate;
varying highp vec2 leftTextureCoordinate;
varying highp vec2 rightTextureCoordinate;
varying highp vec2 topTextureCoordinate;
varying highp vec2 bottomTextureCoordinate;
 
varying highp float centerMultiplier;
varying highp float edgeMultiplier;

uniform sampler2D inputImageTexture;

void main() { 
    mediump vec3 textureColor = texture2D(inputImageTexture, textureCoordinate).rgb;
    mediump vec3 leftTextureColor = texture2D(inputImageTexture, leftTextureCoordinate).rgb;
    mediump vec3 rightTextureColor = texture2D(inputImageTexture, rightTextureCoordinate).rgb;
    mediump vec3 topTextureColor = texture2D(inputImageTexture, topTextureCoordinate).rgb;
    mediump vec3 bottomTextureColor = texture2D(inputImageTexture, bottomTextureCoordinate).rgb;

    gl_FragColor = vec4((textureColor * centerMultiplier - (leftTextureColor * edgeMultiplier + rightTextureColor * edgeMultiplier + topTextureColor * edgeMultiplier + bottomTextureColor * edgeMultiplier)), texture2D(inputImageTexture, bottomTextureCoordinate).w);
}