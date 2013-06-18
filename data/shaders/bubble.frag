uniform sampler2D tex;
uniform float transparency;
varying vec2 uv;

void main()
{
	gl_FragColor = texture2D(tex, uv);
	gl_FragColor.a *= transparency;
}
