#version 330

void main() {
    const vec2 v[3] = vec2[3](
            vec2(-1.0, -1.0),
            vec2(3.0, -1.0),
            vec2(-1.0, 3.0)
        );

    vec2 p = v[gl_VertexID];
    gl_Position = vec4(p, 0.0, 1.0);
}
