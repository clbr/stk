uniform sampler2D tex;
uniform sampler2D dtex;

void main()
{
	vec2 tc = gl_TexCoord[0].xy;
//	vec4 mycol = texture2D(tex, tc);

	vec4 shiftval = texture2D(dtex, tc) / vec4(50.0);
	vec2 shift;
	shift.x = -shiftval.x + shiftval.y;
	shift.y = -shiftval.z + shiftval.w;

	tc += shift;

	vec4 newcol = texture2D(tex, tc);

//	gl_FragColor = mix(mycol, newcol, 0.6);
	gl_FragColor = newcol;
}
