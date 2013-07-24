#version 120

uniform sampler2D tex;
uniform sampler2D oldtex;

const float totStrength = 2.38;
const float strength = 0.07;
const float falloff = 0.000002;
const float rad = 0.006;

#define SAMPLES 16

const float invSamples = 1.0 / SAMPLES.0;

void main(void)
{
	// A set of Random(tm) vec2's. 8 1s, 6 0.7s, 2 0.4
	const vec2 vecs[16] = vec2[](vec2(0.43589, -0.9), vec2(-0.9, 0.43589),
					vec2(-0.8, -0.6), vec2(0.6, 0.8),
					vec2(0.866025, -0.5), vec2(-0.5, 0.866025),
					vec2(-0.3, -0.953939), vec2(0.953939, 0.3),
					vec2(0.3, -0.781025), vec2(-0.781025, 0.3),
					vec2(-0.56, -0.621611), vec2(0.621611, 0.56),
					vec2(0.734847, -0.4), vec2(-0.4, 0.734847),
					vec2(-0.2, -0.6), vec2(0.6, 0.2));

	vec2 uv = gl_TexCoord[0].xy;

	vec4 cur = texture2D(tex, uv);
	float curdepth = cur.a;

	// Will we skip this pixel? (if it's the sky)
	float len = dot(vec3(1.0), cur.xyz);
	if (len < 0.2) discard;

	float mytotstrength = 3.0 * curdepth * totStrength;

	// get the normal of current fragment
	vec3 norm = cur.xyz * vec3(2.0) - vec3(1.0);

	float bl = 0.0;

	// adjust for the depth ( not sure if this is good..)
	float radD = rad/curdepth;

	for(int i = 0; i < SAMPLES; ++i) {

		vec2 ray = uv + radD * vecs[i];

		// get the depth of the occluder fragment
		vec4 occluderFragment = texture2D(tex, ray);

		// get the normal of the occluder fragment
		vec3 occNorm = occluderFragment.xyz * vec3(2.0) - vec3(1.0);

		// if depthDifference is negative = occluder is behind current fragment
		float depthDifference = curdepth - occluderFragment.a;

		// calculate the difference between the normals as a weight
		float normDiff = 1.0 - dot(occNorm, norm);

		// the falloff equation, starts at falloff and is kind of 1/x^2 falling
		bl += step(falloff, depthDifference) * normDiff *
			(1.0 - smoothstep(falloff, strength, depthDifference));
	}

	// output the result
	float ao = 1.0 - mytotstrength * bl * invSamples;

	// Mix with old result to avoid flicker
	float oldao = texture2D(oldtex, uv).x;

	ao = mix(ao, oldao, 0.3);

	gl_FragColor = vec4(vec3(ao), curdepth);
}
