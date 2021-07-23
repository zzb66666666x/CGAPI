layout (location = 0) in vec3 aPos;     
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;  
   
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 projection;    
uniform mat4 view;   
uniform mat4 model;     

void main()
{
    FragPos = vec3(model*vec4(aPos, 1.0));
    gl_Position =  view * vec4(aPos,1.0);
    Normal = vec3(projection * vec4(aNormal, 1.0));
    TexCoord = vec2(666*aTexCoord.x, 666*aTexCoord.y);
}