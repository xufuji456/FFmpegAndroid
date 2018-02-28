#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2;

void main()
{
    vec3 texel = texture2D(inputImageTexture, textureCoordinate).rgb;
    vec2 lookup;
    lookup.y = .5;

    lookup.x = texel.r;
    texel.r = texture2D(inputImageTexture2, lookup).r;

    lookup.x = texel.g;
    texel.g = texture2D(inputImageTexture2, lookup).g;

    lookup.x = texel.b;
    texel.b = texture2D(inputImageTexture2, lookup).b;

    gl_FragColor = vec4(texel, 1.0);
}
