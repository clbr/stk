uniform sampler2D ntex;
uniform sampler2D dtex;

uniform vec3 center;
uniform vec3 col;
uniform vec2 screen;

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

	gl_FragColor = vec4(NdotL * col, 1.0);
}
