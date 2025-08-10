//
//  passthru.glsl
//  
//
//  Created by George Watson on 24/07/2025.
//

@ctype vec2 glm::vec2

@vs passthru_vs
layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;

out vec2 uv;

void main() {
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    uv = texcoord;
}
@end

@fs passthru_fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;

out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end

@program passthru passthru_vs passthru_fs
