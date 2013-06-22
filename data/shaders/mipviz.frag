#version 120

uniform sampler2D tex;
uniform vec2 texsize;

float miplevel(in vec2 texture_coordinate)
{
    // The OpenGL Graphics System: A Specification 4.2
    //  - chapter 3.9.11, equation 3.21

    vec2  dx_vtc        = dFdx(texture_coordinate);
    vec2  dy_vtc        = dFdy(texture_coordinate);
    float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));

    return 0.5 * log2(delta_max_sqr); // == log2(sqrt(delta_max_sqr));
}

void main() {

	const vec4 levels[6] = vec4[](
		vec4(0.0, 0.0, 1.0, 0.8),
		vec4(0.0, 0.5, 1.0, 0.4),
		vec4(1.0, 1.0, 1.0, 0.0),
		vec4(1.0, 0.7, 0.0, 0.2),
		vec4(1.0, 0.3, 0.0, 0.6),
		vec4(1.0, 0.0, 0.0, 0.8)
		);

	float mip = miplevel(texsize * gl_TexCoord[0].xy);
	mip = max(mip, 5.0);
	int i = int(mip);

	vec4 tcol = texture2D(tex, gl_TexCoord[0].xy);

	vec3 col = mix(tcol.xyz, levels[i].xyz, levels[i].a);

	gl_FragColor = vec4(col, tcol.a);
}
