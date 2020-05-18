R"zzz(
#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 vertex_position;

void main() {
	mat4 mvp = projection * view * model;
	gl_Position = mvp * vec4(vertex_position, 1.0f);
}
)zzz"
