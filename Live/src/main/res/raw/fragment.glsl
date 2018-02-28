#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;

void main() {
    gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
}