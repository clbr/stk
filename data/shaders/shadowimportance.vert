uniform sampler2D ntex;
uniform sampler2D dtex;
uniform mat4 ipvmat;
uniform mat4 shadowmat;

varying float linearz;
varying vec3 normal;

float decdepth(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

void main()
{
	vec2 texc = gl_Vertex.xy;
	float z = decdepth(texture2D(dtex, texc));

	vec3 tmp = vec3(texc, z);
	tmp = tmp * 2.0 - 1.0;

	vec4 xpos = vec4(tmp, 1.0);
	xpos = ipvmat * xpos;
	xpos.xyz /= xpos.w;

	// Now we have this pixel's world-space position. Convert to shadow space.
	vec4 pos = shadowmat * vec4(xpos.xyz, 1.0);

	vec4 ntmp = texture2D(ntex, texc);
	normal = ntmp.xyz * 2.0 - 1.0;
	linearz = ntmp.a;

	gl_Position = pos;
}
