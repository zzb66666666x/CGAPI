in vec2 texcoord;

uniform sampler2D texture_diffuse1;

void main(){
    gl_FragColor = texture(texture_diffuse1, texcoord);
}