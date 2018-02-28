varying highp vec2 textureCoordinate;

uniform sampler2D inputImageTexture;
uniform highp float exposure;

void main() {
    highp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
    gl_FragColor = vec4(textureColor.rgb * pow(2.0, exposure), textureColor.w);
}