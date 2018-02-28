#extension GL_OES_EGL_image_external : require

precision highp float;

uniform samplerExternalOES inputImageTexture;
uniform vec2 singleStepOffset;

varying vec2 textureCoordinate;

const vec4 params = vec4(0.748, 0.874, 0.241, 0.241);
const vec3 W = vec3(0.299,0.587,0.114);
const mat3 saturateMatrix = mat3(
                                1.1102,-0.0598,-0.061,
                                -0.0774,1.0826,-0.1186,
                                -0.0228,-0.0228,1.1772);

vec2 blurCoordinates[24];

float hardLight(float color) {
    if(color <= 0.5) {
        color = color * color * 2.0;
    } else {
        color = 1.0 - ((1.0 - color)*(1.0 - color) * 2.0);
    }
    return color;
}

void main() {
    vec3 centralColor = texture2D(inputImageTexture, textureCoordinate).rgb;

    blurCoordinates[0] = textureCoordinate.xy + singleStepOffset * vec2(0.0, -10.0);
    blurCoordinates[1] = textureCoordinate.xy + singleStepOffset * vec2(0.0, 10.0);
    blurCoordinates[2] = textureCoordinate.xy + singleStepOffset * vec2(-10.0, 0.0);
    blurCoordinates[3] = textureCoordinate.xy + singleStepOffset * vec2(10.0, 0.0);
    blurCoordinates[4] = textureCoordinate.xy + singleStepOffset * vec2(5.0, -8.0);
    blurCoordinates[5] = textureCoordinate.xy + singleStepOffset * vec2(5.0, 8.0);
    blurCoordinates[6] = textureCoordinate.xy + singleStepOffset * vec2(-5.0, 8.0);
    blurCoordinates[7] = textureCoordinate.xy + singleStepOffset * vec2(-5.0, -8.0);
    blurCoordinates[8] = textureCoordinate.xy + singleStepOffset * vec2(8.0, -5.0);
    blurCoordinates[9] = textureCoordinate.xy + singleStepOffset * vec2(8.0, 5.0);
    blurCoordinates[10] = textureCoordinate.xy + singleStepOffset * vec2(-8.0, 5.0);
    blurCoordinates[11] = textureCoordinate.xy + singleStepOffset * vec2(-8.0, -5.0);
    blurCoordinates[12] = textureCoordinate.xy + singleStepOffset * vec2(0.0, -6.0);
    blurCoordinates[13] = textureCoordinate.xy + singleStepOffset * vec2(0.0, 6.0);
    blurCoordinates[14] = textureCoordinate.xy + singleStepOffset * vec2(6.0, 0.0);
    blurCoordinates[15] = textureCoordinate.xy + singleStepOffset * vec2(-6.0, 0.0);
    blurCoordinates[16] = textureCoordinate.xy + singleStepOffset * vec2(-4.0, -4.0);
    blurCoordinates[17] = textureCoordinate.xy + singleStepOffset * vec2(-4.0, 4.0);
    blurCoordinates[18] = textureCoordinate.xy + singleStepOffset * vec2(4.0, -4.0);
    blurCoordinates[19] = textureCoordinate.xy + singleStepOffset * vec2(4.0, 4.0);
    blurCoordinates[20] = textureCoordinate.xy + singleStepOffset * vec2(-2.0, -2.0);
    blurCoordinates[21] = textureCoordinate.xy + singleStepOffset * vec2(-2.0, 2.0);
    blurCoordinates[22] = textureCoordinate.xy + singleStepOffset * vec2(2.0, -2.0);
    blurCoordinates[23] = textureCoordinate.xy + singleStepOffset * vec2(2.0, 2.0);

    float sampleColor = centralColor.g * 22.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[0]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[1]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[2]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[3]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[4]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[5]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[6]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[7]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[8]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[9]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[10]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[11]).g;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[12]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[13]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[14]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[15]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[16]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[17]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[18]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[19]).g * 2.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[20]).g * 3.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[21]).g * 3.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[22]).g * 3.0;
    sampleColor += texture2D(inputImageTexture, blurCoordinates[23]).g * 3.0;
    sampleColor = sampleColor / 62.0;

    float highPass = centralColor.g - sampleColor + 0.5;

    for(int i = 0; i < 5;i++)
    {
        highPass = hardLight(highPass);
    }
    float luminance = dot(centralColor, W);
    float alpha = pow(luminance, params.r);

    vec3 smoothColor = centralColor + (centralColor-vec3(highPass))*alpha*0.1;

    smoothColor.r = clamp(pow(smoothColor.r, params.g),0.0,1.0);
    smoothColor.g = clamp(pow(smoothColor.g, params.g),0.0,1.0);
    smoothColor.b = clamp(pow(smoothColor.b, params.g),0.0,1.0);

    vec3 screen = vec3(1.0) - (vec3(1.0)-smoothColor) * (vec3(1.0)-centralColor);
    vec3 lighten = max(smoothColor, centralColor);
    vec3 softLight = 2.0 * centralColor*smoothColor + centralColor*centralColor
                     - 2.0 * centralColor*centralColor * smoothColor;

    gl_FragColor = vec4(mix(centralColor, screen, alpha), 1.0);
    gl_FragColor.rgb = mix(gl_FragColor.rgb, lighten, alpha);
    gl_FragColor.rgb = mix(gl_FragColor.rgb, softLight, params.b);

    vec3 satColor = gl_FragColor.rgb * saturateMatrix;
    gl_FragColor.rgb = mix(gl_FragColor.rgb, satColor, params.a);

    gl_FragColor.rgb = vec3(gl_FragColor.rgb + vec3(-0.096));
}