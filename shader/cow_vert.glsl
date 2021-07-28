layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec2 texcoord;
out vec3 normal;
out vec3 fragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 inv_model;

void main() {
    fragPos = vec3(model * vec4(aPos, 1.0f));
    gl_Position = projection * view * vec4(fragPos,1.0f);
    normal = vec3(inv_model * vec4(aNormal, 0.0f));
    texcoord = aTexCoord;
}