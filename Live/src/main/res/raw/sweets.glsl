#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform lowp samplerExternalOES inputImageTexture;
uniform lowp sampler2D curve;
uniform lowp sampler2D samplerMask;
uniform lowp int lowPerformance;

uniform float texelWidthOffset ;
uniform float texelHeightOffset;

varying mediump vec2 textureCoordinate;

vec4 sharpen(sampler2D sampler) 
{ 
	vec4 color = texture2D(sampler, textureCoordinate) * 2.; 
	
	color -= texture2D(sampler, textureCoordinate-vec2(texelWidthOffset, 0. )) * 0.25;
	color -= texture2D(sampler, textureCoordinate+vec2(texelWidthOffset, 0. )) * 0.25; 
	color -= texture2D(sampler, textureCoordinate-vec2(0., texelHeightOffset)) * 0.25; 
	color -= texture2D(sampler, textureCoordinate+vec2(0., texelHeightOffset)) * 0.25; 

    return color; 
} 

vec4 gaussianBlur(sampler2D sampler) 
{ 
	lowp float strength = 1.; 
	
	vec4 color = vec4(0.); 
	vec2 step  = vec2(0.);
	
	color += texture2D(sampler,textureCoordinate)* 0.0443 ; 
	
	step.x = 1.49583 * texelWidthOffset  * strength; 
	step.y = 1.49583 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+vec2(step.x, 0.)) * 0.04321; 
	color += texture2D(sampler,textureCoordinate-vec2(step.x, 0.)) * 0.04321; 
	color += texture2D(sampler,textureCoordinate+vec2(0., step.y)) * 0.04321; 
	color += texture2D(sampler,textureCoordinate-vec2(0., step.y)) * 0.04321; 
	
	step.x = 2.4719250988753685 * texelWidthOffset  * strength; 
	step.y = 2.4719250988753685 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+step) * 0.041795; 
	color += texture2D(sampler,textureCoordinate-step) * 0.041795; 
	color += texture2D(sampler,textureCoordinate+vec2(-step.x, step.y)) * 0.041795; 
	color += texture2D(sampler,textureCoordinate+vec2( step.x,-step.y)) * 0.041795; 
	
	step.x = 5.49583 * texelWidthOffset  * strength; 
	step.y = 5.49583 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+vec2(step.x, 0.)) * 0.040425; 
	color += texture2D(sampler,textureCoordinate-vec2(step.x, 0.)) * 0.040425; 
	color += texture2D(sampler,textureCoordinate+vec2(0., step.y)) * 0.040425; 
	color += texture2D(sampler,textureCoordinate-vec2(0., step.y)) * 0.040425; 

	step.x = 5.300352223621558 * texelWidthOffset  * strength; 
	step.y = 5.300352223621558 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+step) * 0.0391; 
	color += texture2D(sampler,textureCoordinate-step) * 0.0391; 
	color += texture2D(sampler,textureCoordinate+vec2(-step.x, step.y)) * 0.0391; 
	color += texture2D(sampler,textureCoordinate+vec2( step.x,-step.y)) * 0.0391; 

	step.x = 9.49583 * texelWidthOffset  * strength; 
	step.y = 9.49583 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+vec2(step.x, 0.)) * 0.037815; 
	color += texture2D(sampler,textureCoordinate-vec2(step.x, 0.)) * 0.037815; 
	color += texture2D(sampler,textureCoordinate+vec2(0., step.y)) * 0.037815; 
	color += texture2D(sampler,textureCoordinate-vec2(0., step.y)) * 0.037815; 
	
	step.x = 8.128779348367749 * texelWidthOffset  * strength; 
	step.y = 8.128779348367749 * texelHeightOffset * strength; 
	color += texture2D(sampler,textureCoordinate+step) * 0.03658; 
	color += texture2D(sampler,textureCoordinate-step) * 0.03658; 
	color += texture2D(sampler,textureCoordinate+vec2(-step.x, step.y)) * 0.03658; 
	color += texture2D(sampler,textureCoordinate+vec2( step.x,-step.y)) * 0.03658; 

	return color; 
} 

vec4 level(vec4 color, sampler2D sampler) 
{ 
	color.r = texture2D(sampler, vec2(color.r, 0.)).r; 
	color.g = texture2D(sampler, vec2(color.g, 0.)).g;
	color.b = texture2D(sampler, vec2(color.b, 0.)).b; 

	return color; 
} 
   
vec4 normal(vec4 c1, vec4 c2, float alpha) 
{ 
	return (c2-c1) * alpha + c1;
}   

vec4 lighten(vec4 c1, vec4 c2) 
{ 
	return max(c1,c2);
}

vec4 overlay(vec4 c1, vec4 c2)
{
	vec4 r1 = vec4(0.,0.,0.,1.); 
	r1.r = c1.r < 0.5 ? 2.0*c1.r*c2.r : 1.0 - 2.0*(1.0-c1.r)*(1.0-c2.r);
	r1.g = c1.g < 0.5 ? 2.0*c1.g*c2.g : 1.0 - 2.0*(1.0-c1.g)*(1.0-c2.g);
	r1.b = c1.b < 0.5 ? 2.0*c1.b*c2.b : 1.0 - 2.0*(1.0-c1.b)*(1.0-c2.b);
	
	return r1;
} 

vec3 lerp (vec3 x, vec3 y, float s) 
{
	return x+s*(y-x);
} 

vec4 adjust (vec4 color, float brightness, float contrast, float saturation)
{
	vec3 averageLuminance = vec3(0.5);
	vec3 brightedColor    = color.rgb * (brightness+1.);
	vec3 intensity        = vec3(dot(brightedColor, vec3(0.299, 0.587, 0.114)));
	vec3 saturatedColor   = lerp(intensity, brightedColor, saturation+1.);
	vec3 contrastedColor  = lerp(averageLuminance, saturatedColor, contrast+1.);
	
	return vec4(contrastedColor,1.); 
}

vec4 vibrance(vec4 color, float strength)
{ 
	float luminance = (color.r+color.g+color.b)/3.;
	//dot(color.rgb, vec3(0.299,0.587,0.114)); 
	float maximum   = max(color.r, max(color.g, color.b));
	float amount    = (maximum-luminance)*-strength; 
	
	return vec4(color.rgb * (1.-amount) + maximum*amount, 1.); 
} 
  
void main() 
{ 
	vec4 c1; 
	vec4 c2; 
	if (lowPerformance == 1) 
	{ 	
		c1 = texture2D(inputImageTexture, textureCoordinate); 	
		c2 = texture2D(inputImageTexture, textureCoordinate); 
    } 
	else 
	{ 
		c1 = sharpen(inputImageTexture); 
		c2 = normal(c1, gaussianBlur(inputImageTexture), 0.8); // radius = 13. sharpen?? gaussian blur? ???? ??, ?? blending?? ?? 
	} 
	vec4 c3 = normal(c1, lighten(c1,c2), 0.6); // lighten (0.6) 
    c3 = adjust(c3, 0.12, 0., 0.05); // brightness = 12, saturation = 0.5; 
    c3 = vibrance(level(c3, curve), 0.5); // vibrance = 0.5; 
	c3 = normal(c3, overlay(c3, vec4(0.)), 1.-texture2D(samplerMask, textureCoordinate).r); // vignetting 
	
	gl_FragColor = c3;
}