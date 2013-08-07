varying float realz;
varying float shadowz;

void main()
{
	gl_FragColor = vec4((1.0 - realz) + 0.05);
}
