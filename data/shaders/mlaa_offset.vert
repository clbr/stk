varying vec4 offset[2];
uniform vec2 PIXEL_SIZE;

void main() {
	gl_Position = ftransform();
	vec4 invy = gl_MultiTexCoord0;
//	invy.y = 1.0 - invy.y;
	gl_TexCoord[0] = invy;

	offset[0] = invy.xyxy + PIXEL_SIZE.xyxy * vec4(-1.0, 0.0, 0.0,  1.0);
	offset[1] = invy.xyxy + PIXEL_SIZE.xyxy * vec4( 1.0, 0.0, 0.0, -1.0);
}
