#version 130

// Variables coming into vertex shader from CPP program
in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

//  Variables going to fragment shader
out vec2 texCoord;
out vec3 cameraPosition;    //  from camera to vertex
out vec3 vectorToCamera;    //  from vertex to camera
out vec3 vectorToLight1;    //  from vertex to light1
out vec3 vectorToLight2;    //  from vertex to light2

//  Unchanged variables
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;

void main(){
    // Transform vertex position into eye coordinates
    vec3 pos = (ModelView * vPosition).xyz;
    // The vectors to the lights from the vertex    
    vectorToLight1 = LightPosition1.xyz - pos;
    vectorToLight2 = LightPosition2.xyz - pos;
    // Camera position from vertex is negative of pos
    vectorToCamera = -pos;
    // Transform vertex normal into camera coordinates
    // Assume scaling is unifrom across dimensions
    cameraPosition = (ModelView * vec4(vNormal, 0.0)).xyz;
    
    gl_Position = Projection * ModelView * vPosition;
    texCoord = vTexCoord;
}
