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

    vec3 centralColor = texture2D(inputImageTexture, xy).rgb;
    gl_FragColor = vec4(centralColor.rgb,1.0);
}