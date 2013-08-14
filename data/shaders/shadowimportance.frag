uniform sampler2D ctex;
uniform vec3 campos;

varying vec3 wpos;
varying vec3 normal;
varying float linearz;
varying vec2 texc;

float luminanceImp()
{
	const vec3 weights = vec3(0.2126, 0.7152, 0.0722); // ITU-R BT. 709
	vec3 col = texture2D(ctex, texc).xyz;

	float luma = dot(weights, col);

	// Dark surfaces need less resolution
	float f = smoothstep(0.1, 0.4, luma);
	f = max(0.05, f);

	return f;
}

float normalImp()
{
	vec3 camdir = normalize(campos - wpos);
	vec3 N = normalize(normal);

	// Boost surfaces facing the viewer directly
	float f = 2.0 * max(0.0, dot(N, camdir));

	return f;
}

float depthImp()
{
/*	const float skip = 0.7;

	float f = min(linearz, skip);
	f *= 1.0/skip;*/

	float f = 1.0 - (linearz * 0.9);

	return f;
}

void main()
{
	float importance = normalImp() * depthImp() * luminanceImp();
	importance = clamp(importance, 0.0, 1.0);

	gl_FragColor = vec4(importance);
	gl_FragDepth = 1.0 - importance;
}
