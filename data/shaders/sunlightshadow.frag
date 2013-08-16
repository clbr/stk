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
uniform float shadowoffset;

float decdepth(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

void main() {

	vec2 texc = gl_FragCoord.xy / screen;
	vec4 depthread = texture2D(dtex, texc);
	float z = decdepth(vec4(depthread.xyz, 0.0));

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
	vec3 shadowcoord = (shadowmat * vec4(xpos.xyz, 1.0)).xyz;
	shadowcoord = (shadowcoord * 0.5) + vec3(0.5);

	vec2 movex = texture2D(warpx, shadowcoord.xy).xy;
	vec2 movey = texture2D(warpy, shadowcoord.xy).xy;
	float dx = -movex.x + movex.y;
	float dy = -movey.x + movey.y;
	shadowcoord.xy += vec2(dx, dy);

	vec4 shadowread = texture2D(shadowtex, shadowcoord.xy);
	float shadowmapz = decdepth(vec4(shadowread.xyz, 0.0));

	float moved = (abs(dx) + abs(dy)) * 0.5;

	float bias = 0.002 * tan(acos(NdotL)); // According to the slope
	bias += smoothstep(0.001, 0.4, moved) * 0.01; // According to the warping
	bias = clamp(bias, 0.001, 0.014);

	// This ID, and four IDs around this must match for a shadow pixel
	float right = texture2D(shadowtex, shadowcoord.xy + vec2(shadowoffset, 0.0)).a;
	float left = texture2D(shadowtex, shadowcoord.xy + vec2(-shadowoffset, 0.0)).a;
	float up = texture2D(shadowtex, shadowcoord.xy + vec2(0.0, shadowoffset)).a;
	float down = texture2D(shadowtex, shadowcoord.xy + vec2(0.0, -shadowoffset)).a;

	float matching = ((right + left + up + down) * 0.25) - shadowread.a;
	matching = abs(matching);

	// If the ID is different, we're in shadow
	outcol *= step(abs(shadowread.a - depthread.a) - matching, 0.004);
	// Otherwise, do a normal biased depth comparison
	outcol *= step(shadowcoord.z, shadowmapz + bias);

/*	outcol.r = (shadowcoord.z - shadowmapz) * 50.0;
	outcol.g = moved;*/

	gl_FragColor = vec4(outcol, 1.0);
}
