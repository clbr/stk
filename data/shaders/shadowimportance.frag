varying vec3 normal;
varying float linearz;

float normalImp()
{
	return 1.0;
}

float depthImp()
{
/*	const float skip = 0.7;

	float f = min(linearz, skip);
	f *= 1.0/skip;*/
	float f = 1.0 - (linearz * 0.9);

	return f;
}

void main()
{
	float importance = normalImp() * depthImp();

	gl_FragColor = vec4(importance);
}
