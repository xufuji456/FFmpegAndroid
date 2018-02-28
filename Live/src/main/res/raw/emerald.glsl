#extension GL_OES_EGL_image_external : require

precision highp float;

varying highp vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D curve; 

vec3 RGBtoHSL(vec3 c) { 
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0); 
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g)); 
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r)); 
	
	float d = q.x - min(q.w, q.y); 
	float e = 1.0e-10; 
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x); 
} 

vec3 HSLtoRGB(vec3 c) { 
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0); 
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www); 
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
} 

void main() { 
	float GreyVal;
	highp vec4 textureColor; 
	float xCoordinate = textureCoordinate.x; 
	float yCoordinate = textureCoordinate.y;

	highp float redCurveValue; 
	highp float greenCurveValue; 
    highp float blueCurveValue; 

	textureColor = texture2D( inputImageTexture, vec2(xCoordinate, yCoordinate)); 
	
	// step1 curve 
	redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r; 
	greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g; 
	blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b; 
	vec3 tColor = vec3(redCurveValue, greenCurveValue, blueCurveValue); 
	tColor = RGBtoHSL(tColor); 
	tColor = clamp(tColor, 0.0, 1.0); 

	tColor.g = tColor.g * 1.5; 

	float dStrength = 1.0; 
	float dSatStrength = 0.15; 
    float dHueStrength = 0.08; 

	float dGap = 0.0; 

	if( tColor.r >= 0.625 && tColor.r <= 0.708)
	{ 
		tColor.r = tColor.r - (tColor.r * dHueStrength); 
        tColor.g = tColor.g + (tColor.g * dSatStrength); 		
	} 
	else if( tColor.r >= 0.542 && tColor.r < 0.625) 
	{ 
		dGap = abs(tColor.r - 0.542); 
		dStrength = (dGap / 0.0833); 

		tColor.r = tColor.r + (tColor.r * dHueStrength * dStrength); 
		tColor.g = tColor.g + (tColor.g * dSatStrength * dStrength); 
	} 
	else if( tColor.r > 0.708 && tColor.r <= 0.792)
	{ 
		dGap = abs(tColor.r - 0.792); 
		dStrength = (dGap / 0.0833);

		tColor.r = tColor.r + (tColor.r * dHueStrength * dStrength);
		tColor.g = tColor.g + (tColor.g * dSatStrength * dStrength); 
	} 
	
	tColor = HSLtoRGB(tColor); 
	tColor = clamp(tColor, 0.0, 1.0); 
	
	redCurveValue = texture2D(curve, vec2(tColor.r, 1.0)).r; 
	greenCurveValue = texture2D(curve, vec2(tColor.g, 1.0)).r;
	blueCurveValue = texture2D(curve, vec2(tColor.b, 1.0)).r; 

    redCurveValue = texture2D(curve, vec2(redCurveValue, 1.0)).g; 
	greenCurveValue = texture2D(curve, vec2(greenCurveValue, 1.0)).g; 
	blueCurveValue = texture2D(curve, vec2(blueCurveValue, 1.0)).g; 

	textureColor = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0); 
    gl_FragColor = vec4(textureColor.r, textureColor.g, textureColor.b, 1.0); 
}
