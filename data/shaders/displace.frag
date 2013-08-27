uniform sampler2D dtex;
uniform vec2 screen;
uniform float tick;

varying vec3 lpos;
varying vec3 lnorm;

float decdepth(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

void main()
{
	vec2 tc = gl_FragCoord.xy / screen;
	vec2 origtc = tc;
	float camdist = length(lpos);
	vec3 camdir = normalize(-lpos);
	vec3 normal = normalize(lnorm);
	float angle = dot(normal, camdir);
	vec4 col = vec4(0.0);
	const int steps = 8;
	const float stepmulti = 1.0 / float(steps);
	const float maxlen = 0.04 * stepmulti;

	// Reflection + refraction, but we do refraction only
	vec2 newdir = normalize(refract(camdir, normal, 0.661).xy) * maxlen;

	for (int i = 0; i < steps; i++)
	{
		tc += newdir;
		float depth = decdepth(vec4(texture2D(dtex, tc).xyz, 0.0));

		if (depth < gl_FragCoord.z)
		{
			break;
		}
	}

	// Fade the edges of the screen out
	float fade = smoothstep(0.0, 0.2, origtc.x) *
			(1.0 - smoothstep(0.8, 1.0, origtc.x)) *
			smoothstep(0.0, 0.2, origtc.y) *
			(1.0 - smoothstep(0.8, 1.0, origtc.y));

	// Fade according to distance to cam
	fade *= 1.0 - smoothstep(1.0, 40.0, camdist);

	vec2 offset = tc - origtc;
	offset *= 10.0 * fade * tick;

	col.r = step(offset.x, 0.0) * -offset.x;
	col.g = step(0.0, offset.x) * offset.x;
	col.b = step(offset.y, 0.0) * -offset.y;
	col.a = step(0.0, offset.y) * offset.y;

	gl_FragColor = col;
}
