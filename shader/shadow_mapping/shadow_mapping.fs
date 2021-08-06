in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = vec3(fragPosLightSpace) / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5f + 0.5f;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    vec2 shadow_map_texcoord = vec2(projCoords);
    // if (shadow_map_texcoord.x <= 0 | shadow_map_texcoord.y <= 0 || shadow_map_texcoord.x >= 1 || shadow_map_texcoord.y >= 1)
    //     return 0;
    float closestDepth = texture(shadowMap, shadow_map_texcoord).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0f : 0.0f;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0f){
        shadow = 0.0f;
    }
    return shadow;
}

void main()
{           
    vec3 color = vec3(texture(diffuseTexture, TexCoords));
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(0.3f);
    // ambient
    vec3 ambient = 0.3f * color;
    // diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0f);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0f;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0f), 64.0f);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);                     
    vec3 lighting = (ambient + (1.0f - shadow) * (diffuse + specular)) * color;    
    
    gl_FragColor = vec4(lighting, 1.0f);
}