#version 130
//  -----Part G----
//  Moved the lighting calculations to the fragment shader


//  Variables coming in from vertex shader
in vec3 cameraPosition;	//  from camera to vertex
in vec3 vectorToCamera;	//  from vertex to camera
in vec3 vectorToLight;	//  from vertex to light
in vec2 texCoord;		//	The third coordinate is always 0.0 and is discarded

//  Variable output by fragment shader
out vec4 fColor;
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
    vec3 N = normalize( cameraPosition );	// Normal vertex from camera perspective
    vec3 E = normalize( vectorToCamera );   // Direction to the eye/camera
    vec3 L = normalize( vectorToLight );   	// Direction to the light source
    vec3 H = normalize( L + E );  			// Halfway vector

    // Compute terms in the illumination equation
    vec3 ambient = (lightColour * lightBrightness) * AmbientProduct;

    float Kd = max( dot(L, N), 0.0 );
    vec3  diffuse = Kd * (lightColour * lightBrightness) * DiffuseProduct;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    vec3  specular = Ks * lightBrightness* SpecularProduct;
    // If light is negative distance, no specularity
    if (dot(L, N) < 0.0 ) {
	   specular = vec3(0.0, 0.0, 0.0);
    }

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    // ----Part F----
    // Distance for dropoff of light defined
    float lightDistance = sqrt(dot(vectorToLight, vectorToLight))/15 + 1;
    //float lightDistance = 0.01 + length(vectorToLight);

    // ----Part H----
    // Only ambience, global and diffusion are passed to rgb component
    // colour.a = 1.0 (full opacity)
    vec4 colour = vec4(globalAmbient + ((ambient + diffuse)/lightDistance), 1.0);
    // Specular component always shines white, separate from colour
    // Specular transfers information on degree of shininess
    // Specular is independent of texture colours, added separately
    fColor = colour * texture2D( texture, texCoord * texScale) + vec4(specular / lightDistance, 1.0);
}
