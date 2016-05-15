#version 130


// Variables coming into vertex shader from CPP program
in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
// ----Part2D A1----
in ivec4 boneIDs;
in vec4 boneWeights;

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
uniform mat4 boneTransforms[64];    // ----Part2D A1 variables----

void main(){
    // ----Part 2D A2----
    mat4 boneTransform = boneWeights[0] * boneTransforms[boneIDs[0]] +
                            boneWeights[1] * boneTransforms[boneIDs[1]] +
                                boneWeights[2] * boneTransforms[boneIDs[2]] +
                                    boneWeights[3] * boneTransforms[boneIDs[3]];

    vec4 transformedPosition = boneTransform * vPosition;
    vec3 transformedNormal = mat3(boneTransform) * vNormal;
    

    // Transform vertex position into eye coordinates
    // ----Part 2D A3----
    vec3 pos = (ModelView * transformedPosition).xyz;
    // The vectors to the lights from the vertex    
    vectorToLight1 = LightPosition1.xyz - pos;
    vectorToLight2 = LightPosition2.xyz - pos;
    // Camera position from vertex is negative of pos
    vectorToCamera = -pos;
    // Transform vertex normal into camera coordinates
    // Assume scaling is uniformm across dimensions
    // Part 2D A3
    cameraPosition = (ModelView * vec4(transformedNormal, 0.0)).xyz; 
     // Part 2D A3
    gl_Position = Projection * ModelView * transformedPosition;
    texCoord = vTexCoord;
}
