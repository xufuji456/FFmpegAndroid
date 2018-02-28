#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2;

void main()
{
    vec3 texel = texture2D(inputImageTexture, textureCoordinate).rgb;
    texel = vec3(dot(vec3(0.3, 0.6, 0.1), texel));
    texel = vec3(texture2D(inputImageTexture2, vec2(texel.r, .16666)).r);
    gl_FragColor = vec4(texel, 1.0);
}