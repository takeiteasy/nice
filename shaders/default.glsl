//
//  default.glsl
//
//
//  Created by George Watson on 24/07/2025.
//

@vs default_vs
layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;

layout(binding=0) uniform vs_params {
    mat4 mvp;
};

out vec2 uv;

void main() {
    gl_Position = mvp * vec4(position.x, position.y, 0.0, 1.0);
    uv = texcoord;
}
@end

@fs default_fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;

out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end

@program default_program default_vs default_fs
