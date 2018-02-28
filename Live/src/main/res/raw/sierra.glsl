#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2; //blowout;
uniform sampler2D inputImageTexture3; //overlay;
uniform sampler2D inputImageTexture4; //map

uniform float strength;

void main()
{
    vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
    vec4 texel = texture2D(inputImageTexture, textureCoordinate);
    vec3 bbTexel = texture2D(inputImageTexture2, textureCoordinate).rgb;

    texel.r = texture2D(inputImageTexture3, vec2(bbTexel.r, texel.r)).r;
    texel.g = texture2D(inputImageTexture3, vec2(bbTexel.g, texel.g)).g;
    texel.b = texture2D(inputImageTexture3, vec2(bbTexel.b, texel.b)).b;

    vec4 mapped;
    mapped.r = texture2D(inputImageTexture4, vec2(texel.r, .16666)).r;
    mapped.g = texture2D(inputImageTexture4, vec2(texel.g, .5)).g;
    mapped.b = texture2D(inputImageTexture4, vec2(texel.b, .83333)).b;
    mapped.a = 1.0;

    mapped.rgb = mix(originColor.rgb, mapped.rgb, strength);
    gl_FragColor = mapped;
}