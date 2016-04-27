#version 130

//  Variables coming in from vertex shader
in vec3 cameraNormal;	//  from camera to vertex
in vec3 vectorToCamera;	//  from vertex to camera
in vec3 vectorToLight;	//  from vertex to light
in vec2 texCoord;		//	The third coordinate is always 0.0 and is discarded

//out vec4 color;
vec4 color;

uniform vec3 lightColour;
uniform vec3 AmbientProduct;
uniform vec3 DiffuseProduct;
uniform vec3 SpecularProduct;
uniform float lightBrightness;
uniform float Shininess;
uniform float texScale;
uniform sampler2D texture;

void main(){
    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( vectorToLight );   		// Direction to the light source
    vec3 E = normalize( vectorToCamera );   	// Direction to the eye/camera
    vec3 H = normalize( vectorToLight + E );  	// Halfway vector
    vec3 N = normalize( cameraNormal );			// Normal vertex from camera perspective

    // Compute terms in the illumination equation
    vec3 ambient = (lightColour * lightBrightness) * AmbientProduct;

    float Kd = max( dot(L, N), 0.0 );
    vec3  diffuse = Kd * (lightColour * lightBrightness) * DiffuseProduct;

    float Ks = pow( max(dot(N, H), 0.0), Shininess ); // specular light
    vec3  specular = Ks * SpecularProduct;
    // If diffuse light < 0.0
    if (dot(L, N) < 0.0 ) {
	   specular = vec3(0.0, 0.0, 0.0);
    } 

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    // ----Part H---- separate specular from colour
    color.rgb = globalAmbient  + ambient + diffuse; 
    color.a = 1.0;

    gl_FragColor = color * texture2D( texture, texCoord * 2.0 );
}
