@vs default_vs
in vec2 position;
in vec2 texcoord;
in vec4 color;

out vec2 uv;
out vec4 col;

void main() {
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    uv = texcoord;
    col = color;
}
@end

@fs default_fs
uniform texture2D tex;
uniform sampler smp;
in vec2 uv;
in vec4 col;

out vec4 fragColor;

void main() {
    fragColor = texture(sampler2D(tex, smp), uv) * col;
}
@end

@program default_program default_vs default_fs
