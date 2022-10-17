#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;

void main(){

    vec3 centralColor = texture2D(inputImageTexture, textureCoordinate).rgb;
    gl_FragColor = vec4(centralColor.rgb,1.0);;
}