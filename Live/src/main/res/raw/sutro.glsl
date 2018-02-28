#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2; //sutroMap;
uniform sampler2D inputImageTexture3; //sutroMetal;
uniform sampler2D inputImageTexture4; //softLight
uniform sampler2D inputImageTexture5; //sutroEdgeburn
uniform sampler2D inputImageTexture6; //sutroCurves

uniform float strength;

void main()
{
    vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
    vec3 texel = texture2D(inputImageTexture, textureCoordinate).rgb;

    vec2 tc = (2.0 * textureCoordinate) - 1.0;
    float d = dot(tc, tc);
    vec2 lookup = vec2(d, texel.r);
    texel.r = texture2D(inputImageTexture2, lookup).r;
    lookup.y = texel.g;
    texel.g = texture2D(inputImageTexture2, lookup).g;
    lookup.y = texel.b;
    texel.b	= texture2D(inputImageTexture2, lookup).b;

    vec3 rgbPrime = vec3(0.1019, 0.0, 0.0);
    float m = dot(vec3(.3, .59, .11), texel.rgb) - 0.03058;
    texel = mix(texel, rgbPrime + m, 0.32);

    vec3 metal = texture2D(inputImageTexture3, textureCoordinate).rgb;
    texel.r = texture2D(inputImageTexture4, vec2(metal.r, texel.r)).r;
    texel.g = texture2D(inputImageTexture4, vec2(metal.g, texel.g)).g;
    texel.b = texture2D(inputImageTexture4, vec2(metal.b, texel.b)).b;

    texel = texel * texture2D(inputImageTexture5, textureCoordinate).rgb;

    texel.r = texture2D(inputImageTexture6, vec2(texel.r, .16666)).r;
    texel.g = texture2D(inputImageTexture6, vec2(texel.g, .5)).g;
    texel.b = texture2D(inputImageTexture6, vec2(texel.b, .83333)).b;

    texel.rgb = mix(originColor.rgb, texel.rgb, strength);

    gl_FragColor = vec4(texel, 1.0);
}