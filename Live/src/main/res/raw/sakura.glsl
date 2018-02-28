#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform samplerExternalOES inputImageTexture;
uniform sampler2D curve; 
uniform float texelWidthOffset; 
uniform float texelHeightOffset; 

varying mediump vec2 textureCoordinate; 

vec4 gaussianBlur(sampler2D sampler) { 
	lowp float strength = 1.; 
	vec4 color = vec4(0.); 
	vec2 step  = vec2(0.); 

	color += texture2D(sampler,textureCoordinate)* 0.25449 ; 
	
	step.x = 1.37754 * texelWidthOffset  * strength; 
	step.y = 1.37754 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+step) * 0.24797; 
	color += texture2D(sampler,textureCoordinate-step) * 0.24797; 

	step.x = 3.37754 * texelWidthOffset  * strength; 
	step.y = 3.37754 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+step) * 0.09122; 
	color += texture2D(sampler,textureCoordinate-step) * 0.09122; 

	step.x = 5.37754 * texelWidthOffset  * strength; 
	step.y = 5.37754 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+step) * 0.03356; 
	color += texture2D(sampler,textureCoordinate-step) * 0.03356; 

	return color; 
} 

vec4 overlay(vec4 c1, vec4 c2){ 
	vec4 r1 = vec4(0.,0.,0.,1.); 

	r1.r = c1.r < 0.5 ? 2.0*c1.r*c2.r : 1.0 - 2.0*(1.0-c1.r)*(1.0-c2.r); 
	r1.g = c1.g < 0.5 ? 2.0*c1.g*c2.g : 1.0 - 2.0*(1.0-c1.g)*(1.0-c2.g); 
	r1.b = c1.b < 0.5 ? 2.0*c1.b*c2.b : 1.0 - 2.0*(1.0-c1.b)*(1.0-c2.b); 

	return r1; 
} 

vec4 level0c(vec4 color, sampler2D sampler) { 
    color.r = texture2D(sampler, vec2(color.r, 0.)).r; 
    color.g = texture2D(sampler, vec2(color.g, 0.)).r; 
	color.b = texture2D(sampler, vec2(color.b, 0.)).r; 
	return color; 
} 

vec4 normal(vec4 c1, vec4 c2, float alpha) { 
    return (c2-c1) * alpha + c1; 
} 

vec4 screen(vec4 c1, vec4 c2) { 
	vec4 r1 = vec4(1.) - ((vec4(1.) - c1) * (vec4(1.) - c2)); 
	return r1; 
} 

void main() { 
	// naver skin 
	lowp vec4 c0 = texture2D(inputImageTexture, textureCoordinate);
	lowp vec4 c1 = gaussianBlur(inputImageTexture); 
	lowp vec4 c2 = overlay(c0, level0c(c1, curve)); 
	lowp vec4 c3 = normal(c0,c2,0.15); 

	gl_FragColor = c3; 
}