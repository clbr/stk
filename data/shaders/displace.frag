uniform sampler2D dtex;
uniform vec2 screen;

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
	const float maxlen = 0.25 * stepmulti;

	vec2 newdir = vec2(0.0);

	if (angle > 0.661) // Critical angle of reflection in water-air boundary
	{
		// Reflection + refraction, but we do refraction only
		// We know that going from air to water there is no critical angle,
		// but it looks nice.
		newdir = normalize(refract(camdir, normal, 0.661).xy) * maxlen;
	} else
	{
		// Reflection only
		newdir = normalize(reflect(camdir, normal).xy) * maxlen;
	}

	for (int i = 0; i < steps; i++)
	{
		tc += newdir;
		float depth = decdepth(vec4(texture2D(dtex, tc).xyz, 0.0));

		if (depth < gl_FragCoord.z)
		{
			break;
		}
	}

	vec2 offset = tc - origtc;
	offset *= 4.0;

	col.r = step(offset.x, 0.0) * -offset.x;
	col.g = step(0.0, offset.x) * offset.x;
	col.b = step(offset.y, 0.0) * -offset.y;
	col.a = step(0.0, offset.y) * offset.y;

	gl_FragColor = col;
}
