layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 fragPos;
out vec3 normal;
// out vec2 texcoord;

uniform mat4 inv_model;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    fragPos = vec3(model * vec4(aPos, 1.0f));
    
    normal = vec3(inv_model * vec4(aNormal, 0.0f)) * 0.5f + vec3(0.5f, 0.5f, 0.5f);

    gl_Position = projection * view * vec4(fragPos, 1.0);
    // texcoord = aTexCoord;

}