#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D inputImageTexture2; //map
uniform sampler2D inputImageTexture3; //vigMap

uniform float strength;

void main()
{
    vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
    vec3 texel = texture2D(inputImageTexture, textureCoordinate).rgb;

    vec2 tc = (2.0 * textureCoordinate) - 1.0;
    float d = dot(tc, tc);
    vec2 lookup = vec2(d, texel.r);
    texel.r = texture2D(inputImageTexture3, lookup).r;
    lookup.y = texel.g;
    texel.g = texture2D(inputImageTexture3, lookup).g;
    lookup.y = texel.b;
    texel.b	= texture2D(inputImageTexture3, lookup).b;

    vec2 red = vec2(texel.r, 0.16666);
    vec2 green = vec2(texel.g, 0.5);
    vec2 blue = vec2(texel.b, .83333);
    texel.r = texture2D(inputImageTexture2, red).r;
    texel.g = texture2D(inputImageTexture2, green).g;
    texel.b = texture2D(inputImageTexture2, blue).b;
    
    texel.rgb = mix(originColor.rgb, texel.rgb, strength);
     
    gl_FragColor = vec4(texel, 1.0);
}