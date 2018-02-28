#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D curve; 
uniform sampler2D mask;

uniform float texelWidthOffset ;

uniform float texelHeightOffset;

varying mediump vec2 textureCoordinate;

vec4 level0c(vec4 color, sampler2D sampler) 
{ 
	color.r = texture2D(sampler, vec2(color.r, 0.)).r; 
	color.g = texture2D(sampler, vec2(color.g, 0.)).r;
	color.b = texture2D(sampler, vec2(color.b, 0.)).r;
	return color;
} 

vec4 level1c(vec4 color, sampler2D sampler) 
{ 
	color.r = texture2D(sampler, vec2(color.r, 0.)).g;
	color.g = texture2D(sampler, vec2(color.g, 0.)).g;
	color.b = texture2D(sampler, vec2(color.b, 0.)).g;
	return color;
} 

vec4 level2c(vec4 color, sampler2D sampler) 
{ 
	color.r = texture2D(sampler, vec2(color.r,0.)).b; 
	color.g = texture2D(sampler, vec2(color.g,0.)).b;
	color.b = texture2D(sampler, vec2(color.b,0.)).b; 
	return color; 
} 

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

vec4 normal(vec4 c1, vec4 c2, float alpha) 
{ 
	return (c2-c1) * alpha + c1; 
} 

vec4 multiply(vec4 c1, vec4 c2) 
{
	return c1 * c2 * 1.01;
}

vec4 overlay(vec4 c1, vec4 c2)
{
	vec4 color = vec4(0.,0.,0.,1.);
	
	color.r = c1.r < 0.5 ? 2.0*c1.r*c2.r : 1.0 - 2.0*(1.0-c1.r)*(1.0-c2.r);
	color.g = c1.g < 0.5 ? 2.0*c1.g*c2.g : 1.0 - 2.0*(1.0-c1.g)*(1.0-c2.g);
	color.b = c1.b < 0.5 ? 2.0*c1.b*c2.b : 1.0 - 2.0*(1.0-c1.b)*(1.0-c2.b); 

	return color;
}

vec4 screen(vec4 c1, vec4 c2) 
{ 
	return vec4(1.) - ((vec4(1.) - c1) * (vec4(1.) - c2)); 
} 

void main() 
{ 
	// iOS ImageLiveFilter adjustment
	// begin 
	
	vec4 textureColor; 
	
	vec4 t0 = texture2D(mask, vec2(textureCoordinate.x, textureCoordinate.y)); 
	
	// naver skin 
	vec4 c2 = texture2D(inputImageTexture, textureCoordinate); 
	vec4 c5 = c2; 
	
	// healthy 
	vec3 hsv = rgb2hsv(c5.rgb); 
	lowp float h = hsv.x; 
	lowp float s = hsv.y; 
	lowp float v = hsv.z; 
	
	lowp float cF = 0.;   
	// color strength 
	lowp float cG = 0.;   
	// color gap; 
	lowp float sF = 0.06; 
	// saturation strength; 
	
	if(h >= 0.125 && h <= 0.208) 
	{ 
		// 45 to 75 
		s = s - (s * sF); 
	} 
	else if (h >= 0.208 && h < 0.292) 
	{ 
		// 75 to 105 
		cG = abs(h - 0.208); 
		cF = (cG / 0.0833); 
		s = s - (s * sF * cF); 
	} 
	else if (h > 0.042 && h <=  0.125) 
	{ 
		// 15 to 45 
		cG = abs(h - 0.125); 
		cF = (cG / 0.0833); 
		s = s - (s * sF * cF); 
	} 
	hsv.y = s; 
	
	vec4 c6 = vec4(hsv2rgb(hsv),1.); 
	
	c6 = normal(c6, screen  (c6, c6), 0.275); // screen 70./255. 
	c6 = normal(c6, overlay (c6, vec4(1., 0.61176, 0.25098, 1.)), 0.04); // overlay 10./255. 
	
	c6 = normal(c6, multiply(c6, t0), 0.262); // multiply 67./255. 
	
	c6 = level1c(level0c(c6,curve),curve); 
	
	gl_FragColor = c6; 
	// end
} 