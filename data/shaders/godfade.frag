uniform sampler2D tex;

void main()
{
	vec4 col = texture2D(tex, gl_TexCoord[0].xy);

	// Keep the sun fully bright, but fade the sky
	float mul = col.r * col.g * col.b;
	mul = step(mul, 0.99);
	mul *= 0.97;
	mul = 1.0 - mul;

	col = col * vec4(mul);

	gl_FragColor = col;
}
