#extension GL_OES_EGL_image_external : require

precision highp float;

varying highp vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D curve; 

vec3 rgb2hsv(vec3 c) 
{ 
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0); 
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r)); 
	
	float d = q.x - min(q.w, q.y); 
	float e = 1.0e-10; 
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x); 
} 

vec3 hsv2rgb(vec3 c) 
{ 
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0); 
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www); 
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y); 
} 

void main() 
{ 
    float GreyVal; 
	lowp vec4 textureColor; 
	lowp vec4 textureColorOri;
	float xCoordinate = textureCoordinate.x;
	float yCoordinate = textureCoordinate.y; 

	highp float redCurveValue;
	highp float greenCurveValue;
	highp float blueCurveValue; 

	textureColor = texture2D( inputImageTexture, vec2(xCoordinate, yCoordinate)); 

	// step1 20% opacity  ExclusionBlending 
    mediump vec4 textureColor2 = textureColor; 
	textureColor2 = textureColor + textureColor2 - (2.0 * textureColor2 * textureColor); 

	textureColor = (textureColor2 - textureColor) * 0.2 + textureColor; 

    // step2 curve 
    redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r; 
	greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g; 
	blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b; 

    redCurveValue = texture2D(curve, vec2(redCurveValue, 1.0)).r; 
	greenCurveValue = texture2D(curve, vec2(greenCurveValue, 1.0)).r;
	blueCurveValue = texture2D(curve, vec2(blueCurveValue, 1.0)).r; 

	redCurveValue = texture2D(curve, vec2(redCurveValue, 1.0)).g; 
	greenCurveValue = texture2D(curve, vec2(greenCurveValue, 1.0)).g; 
	blueCurveValue = texture2D(curve, vec2(blueCurveValue, 1.0)).g; 


	vec3 tColor = vec3(redCurveValue, greenCurveValue, blueCurveValue); 
	tColor = rgb2hsv(tColor); 

	tColor.g = tColor.g * 0.65; 

	tColor = hsv2rgb(tColor); 
    tColor = clamp(tColor, 0.0, 1.0); 

    mediump vec4 base = vec4(tColor, 1.0); 
	mediump vec4 overlay = vec4(0.62, 0.6, 0.498, 1.0); 
	// step6 overlay blending 
    mediump float ra; 
	if (base.r < 0.5) 
	{ 
		ra = overlay.r * base.r * 2.0;
	} else 
	{ 
		ra = 1.0 - ((1.0 - base.r) * (1.0 - overlay.r) * 2.0);
	}

    mediump float ga; 
	if (base.g < 0.5) 
	{ 
		ga = overlay.g * base.g * 2.0; 
	} else 
	{ 
		ga = 1.0 - ((1.0 - base.g) * (1.0 - overlay.g) * 2.0); 
	} 

	mediump float ba; 
	if (base.b < 0.5) 
	{ 
		ba = overlay.b * base.b * 2.0; 
	} else 
	{ 
		ba = 1.0 - ((1.0 - base.b) * (1.0 - overlay.b) * 2.0); 
	} 
	textureColor = vec4(ra, ga, ba, 1.0); 
	textureColor = (textureColor - base) * 0.1 + base; 

	gl_FragColor = vec4(textureColor.r, textureColor.g, textureColor.b, 1.0); 
} 
	