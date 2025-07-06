// Modern syntax test using vertex_shader/fragment_shader
// This demonstrates the new syntax with top-level uniforms and inputs

uniform mat4 modelViewProj;
uniform mat4 worldMatrix;
uniform vec3 lightDirection;
uniform vec3 lightColor;

input vec3 position;
input vec3 normal;
input vec2 texCoord;

vertex_shader(vec3 position : POSITION, vec3 normal : NORMAL, vec2 texCoord : TEXCOORD0) {
    vec4 worldPos = worldMatrix * vec4(position, 1.0);
    vec3 worldNormal = normalize((worldMatrix * vec4(normal, 0.0)).xyz);
    
    out vec3 v_normal : NORMAL;
    out vec3 v_position : POSITION;
    out vec2 v_texCoord : TEXCOORD0;
    
    v_normal = worldNormal;
    v_position = worldPos.xyz;
    v_texCoord = texCoord;
    
    gl_Position = modelViewProj * vec4(position, 1.0);
}

fragment_shader(vec3 position : POSITION, vec3 normal : NORMAL, vec2 texCoord : TEXCOORD0) {
    vec3 normal = normalize(v_normal);
    vec3 light = normalize(-lightDirection);
    
    float NdotL = max(dot(normal, light), 0.0);
    vec3 diffuse = NdotL * lightColor;
    vec3 ambient = vec3(0.1);
    
    vec3 finalColor = diffuse + ambient;
    fragColor = vec4(finalColor, 1.0);
} 