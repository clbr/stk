uniform sampler2D warpx;
uniform sampler2D warpy;

void main()
{
	vec4 pos = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;

	vec2 tc = pos.xy * vec2(0.5) + vec2(0.5);

	vec2 movex = texture2D(warpx, tc).xy;
	vec2 movey = texture2D(warpy, tc).xy;

	float dx = -movex.x + movex.y;
	float dy = -movey.x + movey.y;

	dx *= 2.0;
	dy *= 2.0;

	gl_Position = pos + vec4(dx, dy, vec2(0.0));
}
