uniform sampler2D tex;
uniform int hastex;

vec4 encdepth(float v) {
	vec4 enc = vec4(1.0, 255.0, 65025.0, 16581375.0) * v;
	enc = fract(enc);
	enc -= enc.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
	return enc;
}

void main() {

	if (hastex != 0) {
		float alpha = texture2D(tex, gl_TexCoord[0].xy).a;

		if (alpha < 0.5)
			discard;
	}

	gl_FragColor = encdepth(gl_FragCoord.z);
}
