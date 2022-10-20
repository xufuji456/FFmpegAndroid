#extension GL_OES_EGL_image_external : require

precision mediump float;

varying mediump vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;

void main(){
    vec2 xy = textureCoordinate.xy;
    // two screen(0.25~0.75)
//    if (xy.x <= 0.5) {
//        xy.x += 0.25;
//    } else {
//        xy.x -= 0.25;
//    }
    // four screen
//    if (xy.x <= 0.5) {
//        xy.x = xy.x * 2.0;
//    } else {
//        xy.x = (xy.x - 0.5) * 2.0;
//    }
//    if (xy.y <= 0.5) {
//        xy.y = xy.y * 2.0;
//    } else {
//        xy.y = (xy.y - 0.5) * 2.0;
//    }
    // white black
//    const vec3 weight = vec3(0.3, 0.59, 0.11);
//    float gray = dot(textureColor.rgb, weight);
    // invert
//    1.0 - textureColor.rgb
    // mirror
//    if (xy.x <= 0.5) {
//        xy.x += 0.25;
//    } else {
//        xy.x -= 0.25;
//        xy.x = 1.0 - xy.x;
//    }

    vec3 textureColor = texture2D(inputImageTexture, xy).rgb;
    gl_FragColor = vec4(textureColor.rgb,1.0);
}