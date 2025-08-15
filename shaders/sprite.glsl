//
//  sprite.glsl
//
//
//  Created by George Watson on 24/07/2025.
//

@ctype mat4 glm::mat4
@ctype vec2 glm::vec2
@ctype vec4 glm::vec4

@vs sprite_vs
layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;

layout(binding=0) uniform sprite_vs_params {
    mat4 mvp;
    vec4 color;
};

out vec2 uv;
out vec4 col;

void main() {
    gl_Position = mvp * vec4(position.x, position.y, 0.0, 1.0);
    uv = texcoord;
    col = color;
}
@end

@fs sprite_fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;
in vec4 col;

out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv) * col;
}
@end

@program sprite sprite_vs sprite_fs
