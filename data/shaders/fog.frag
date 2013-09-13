uniform sampler2D tex;

uniform float fogmax;
uniform float startH;
uniform float endH;
uniform float start;
uniform float end;
uniform vec3 col;
uniform vec3 campos;
uniform mat4 ipvmat;

float decdepth(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

void main()
{
	float z = decdepth(vec4(texture2D(tex, gl_TexCoord[0].xy).xyz, 0.0));

	vec3 tmp = vec3(gl_TexCoord[0].xy, z);
	tmp = tmp * 2.0 - 1.0;

	vec4 xpos = vec4(tmp, 1.0);
	xpos = ipvmat * xpos;
	xpos.xyz /= xpos.w;

	float dist = distance(campos, xpos.xyz);
	float fog = smoothstep(start, end, dist);
	fog *= 1.0 - smoothstep(startH, endH, xpos.y);

	fog = min(fog, fogmax);

	gl_FragColor = vec4(col, fog);
}
