#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2;

void main()
{
    vec3 texel = texture2D(inputImageTexture, textureCoordinate).rgb;
    texel = vec3(
                 texture2D(inputImageTexture2, vec2(texel.r, .16666)).r,
                 texture2D(inputImageTexture2, vec2(texel.g, .5)).g,
                 texture2D(inputImageTexture2, vec2(texel.b, .83333)).b);
    gl_FragColor = vec4(texel, 1.0);
}
