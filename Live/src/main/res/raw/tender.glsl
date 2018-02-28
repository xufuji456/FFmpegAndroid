#extension GL_OES_EGL_image_external : require

precision highp float;

varying highp vec2 textureCoordinate;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D curve;
uniform sampler2D grey1Frame; 

void main()
{ 
	mediump vec4 textureColor;
	mediump vec4 textureColorRes;
	vec4 grey1Color;
	mediump float satVal = 65.0 / 100.0; 
	mediump float mask1R = 29.0 / 255.0; 
	mediump float mask1G = 43.0 / 255.0; 
	mediump float mask1B = 95.0 / 255.0;
	
	highp float xCoordinate = textureCoordinate.x;
	highp float yCoordinate = textureCoordinate.y;
	
	highp float redCurveValue;
	highp float greenCurveValue; 
	highp float blueCurveValue; 

	textureColor = texture2D( inputImageTexture, vec2(xCoordinate, yCoordinate));
	textureColorRes = textureColor;

	grey1Color = texture2D(grey1Frame, vec2(xCoordinate, yCoordinate)); 

	// step1. saturation
    highp float G = (textureColor.r + textureColor.g + textureColor.b); 
	G = G / 3.0; 

	redCurveValue = ((1.0 - satVal) * G + satVal * textureColor.r);
	greenCurveValue = ((1.0 - satVal) * G + satVal * textureColor.g); 
	blueCurveValue = ((1.0 - satVal) * G + satVal * textureColor.b); 

	// step2 curve 
    redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
	greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g;
	blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b;

	// step3 30% opacity  ExclusionBlending
	textureColor = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
	mediump vec4 textureColor2 = vec4(mask1R, mask1G, mask1B, 1.0);
    textureColor2 = textureColor + textureColor2 - (2.0 * textureColor2 * textureColor); 

	textureColor = (textureColor2 - textureColor) * 0.3 + textureColor; 

	mediump vec4 overlay = vec4(0, 0, 0, 1.0); 
	mediump vec4 base = vec4(textureColor.r, textureColor.g, textureColor.b, 1.0); 

	// step4 overlay blending 
	mediump float ra; 
    if (base.r < 0.5) 
	{ 
		ra = overlay.r * base.r * 2.0; 
	} 
	else
	{ 
		ra = 1.0 - ((1.0 - base.r) * (1.0 - overlay.r) * 2.0);
	} 

	mediump float ga; 
	if (base.g < 0.5)
	{ 
		ga = overlay.g * base.g * 2.0;
	} 
	else 
	{ 
		ga = 1.0 - ((1.0 - base.g) * (1.0 - overlay.g) * 2.0); 
	} 

    mediump float ba; 
	if (base.b < 0.5) 
	{ 
		ba = overlay.b * base.b * 2.0; 
	} 
	else 
	{ 
		ba = 1.0 - ((1.0 - base.b) * (1.0 - overlay.b) * 2.0); 
	} 

    textureColor = vec4(ra, ga, ba, 1.0); 
	base = (textureColor - base) * (grey1Color.r/2.0) + base; 

	gl_FragColor = vec4(base.r, base.g, base.b, 1.0);
}
  