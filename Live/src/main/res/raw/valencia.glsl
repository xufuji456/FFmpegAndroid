#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2; //map
uniform sampler2D inputImageTexture3; //gradMap

mat3 saturateMatrix = mat3(
                           1.1402,
                           -0.0598,
                           -0.061,
                           -0.1174,
                           1.0826,
                           -0.1186,
                           -0.0228,
                           -0.0228,
                           1.1772);

vec3 lumaCoeffs = vec3(.3, .59, .11);

uniform float strength;

void main()
{
    vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
    vec3 texel = texture2D(inputImageTexture, textureCoordinate).rgb;

    texel = vec3(
                 texture2D(inputImageTexture2, vec2(texel.r, .1666666)).r,
                 texture2D(inputImageTexture2, vec2(texel.g, .5)).g,
                 texture2D(inputImageTexture2, vec2(texel.b, .8333333)).b
                 );

    texel = saturateMatrix * texel;
    float luma = dot(lumaCoeffs, texel);
    texel = vec3(
                 texture2D(inputImageTexture3, vec2(luma, texel.r)).r,
                 texture2D(inputImageTexture3, vec2(luma, texel.g)).g,
                 texture2D(inputImageTexture3, vec2(luma, texel.b)).b);

    texel.rgb = mix(originColor.rgb, texel.rgb, strength);
    gl_FragColor = vec4(texel, 1.0);
}