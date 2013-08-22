uniform sampler2D tex;
uniform sampler2D caustictex;

void main()
{
	vec2 tc = gl_TexCoord[0].xy;

	vec3 col = texture2D(tex, tc).xyz;
	float caustic = texture2D(caustictex, tc).x;

	col += caustic;

	gl_FragColor = vec4(col, 1.0);
}
