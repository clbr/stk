uniform sampler2D tex;
uniform sampler2D caustictex;
uniform vec2 dir;

void main()
{
	vec2 tc = gl_TexCoord[0].xy;

	vec3 col = texture2D(tex, tc).xyz;
	float caustic = texture2D(caustictex, tc + dir).x;
	float caustic2 = texture2D(caustictex, (tc.yx + dir.yx)).x;

	col += caustic + caustic2;

	gl_FragColor = vec4(col, 1.0);
}
