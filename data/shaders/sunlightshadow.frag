uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D cloudtex;
uniform sampler2D shadowtex;
uniform sampler2D warpx;
uniform sampler2D warpy;

uniform vec3 center;
uniform vec3 col;
uniform vec2 screen;
uniform mat4 invprojview;
uniform mat4 shadowmat;
uniform int hasclouds;
uniform vec2 wind;

float decdepth(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

void main() {

	vec2 texc = gl_FragCoord.xy / screen;
	float z = decdepth(texture2D(dtex, texc));

	if (z < 0.03) discard;

	vec3 norm = texture2D(ntex, texc).xyz;
	norm = (norm - 0.5) * 2.0;

	// Normalized on the cpu
	vec3 L = center;

	float NdotL = max(0.0, dot(norm, L));
	if (NdotL < 0.01) discard;

	vec3 outcol = NdotL * col;

	// World-space position
	vec3 tmp = vec3(texc, z);
	tmp = tmp * 2.0 - 1.0;

	vec4 xpos = vec4(tmp, 1.0);
	xpos = invprojview * xpos;
	xpos.xyz /= xpos.w;

	if (hasclouds == 1)
	{
		vec2 cloudcoord = (xpos.xz * 0.00833333) + wind;
		float cloud = texture2D(cloudtex, cloudcoord).x;
		//float cloud = step(0.5, cloudcoord.x) * step(0.5, cloudcoord.y);

		outcol *= cloud;
	}

	// Shadows
	const float bias = 0.005;
	vec3 shadowcoord = (shadowmat * vec4(xpos.xyz, 1.0)).xyz;
	shadowcoord = (shadowcoord * 0.5) + vec3(0.5);

	vec2 movex = texture2D(warpx, shadowcoord.xy).xy;
	vec2 movey = texture2D(warpy, shadowcoord.xy).xy;
	float dx = -movex.x + movex.y;
	float dy = -movey.x + movey.y;
	shadowcoord.xy += vec2(dx, dy);

	float shadowmapz = decdepth(texture2D(shadowtex, shadowcoord.xy));

	outcol *= step(shadowcoord.z, shadowmapz + bias);

	gl_FragColor = vec4(outcol, 1.0);
}
