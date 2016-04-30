#version 130
//  -----Part G----
//  Moved the lighting calculations to the fragment shader


//  Variables coming in from vertex shader
in vec3 cameraPosition;	//  from camera to vertex
in vec3 vectorToCamera;	//  from vertex to camera
in vec3 vectorToLight1;	//  from vertex to light1
in vec3 vectorToLight2; //	from vertex to light2
in vec2 texCoord;		//	The third coordinate is always 0.0 and is discarded

//  Variable output by fragment shader
out vec4 fColor;
uniform vec3 lightColour1;
uniform vec3 lightColour2;
uniform vec3 AmbientProduct;
uniform vec3 DiffuseProduct;
uniform vec3 SpecularProduct;
uniform float lightBrightness1;
uniform float lightBrightness2;
uniform float Shininess;
uniform float texScale;
uniform sampler2D texture;

void main(){
    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 N = normalize( cameraPosition );	// Normal vertex from camera perspective
    vec3 E = normalize( vectorToCamera );   // Direction to the eye/camera
    vec3 L1 = normalize( vectorToLight1 );  // Direction to the light source 1
    vec3 L2 = normalize( vectorToLight2 );	// Direction to the light source 2
    vec3 H1 = normalize( L1 + E );  		// Halfway vector using light1
    vec3 H2 = normalize( L2 + E );			// Halfway vector using light2

    // Compute terms in the illumination equation
    vec3 ambient1 = (lightColour1 * lightBrightness1) * AmbientProduct;
    vec3 ambient2 = (lightColour2 * lightBrightness2) * AmbientProduct;

    float Kd1 = max( dot(L1, N), 0.0 );
    float Kd2 = max( dot(L2, N), 0.0 );
    vec3  diffuse1 = Kd1 * (lightColour1 * lightBrightness1) * DiffuseProduct;
    vec3  diffuse2 = Kd2 * (lightColour2 * lightBrightness2) * DiffuseProduct;

    float Ks1 = pow( max(dot(N, H1), 0.0), Shininess );
    float Ks2 = pow( max(dot(N, H2), 0.0), Shininess );
    vec3  specular1 = Ks1 * lightBrightness1* SpecularProduct;
    vec3  specular2 = Ks2 * lightBrightness2* SpecularProduct;
    // If light is negative distance, no specularity
    if (dot(L1, N) < 0.0 ) {
	   specular1 = vec3(0.0, 0.0, 0.0);
    }
    if (dot(L2, N) < 0.0 ) {
	   specular2 = vec3(0.0, 0.0, 0.0);
    }

    // globalAmbient is independent of distance from the light sources
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    // ----Part F----
    // Distance for dropoff of light defined
    // Dropoff is only on light 1.
    float lightDistance = sqrt(dot(vectorToLight1, vectorToLight1))/15 + 1;

    // ----Part H----
    // Only ambience, global and diffusion are passed to rgb component
    // colour.a = 1.0 (full opacity)
    vec4 colour = vec4(globalAmbient + ((ambient1 + diffuse1)/lightDistance) + ambient2 + diffuse2, 1.0);
    // Specular component always shines white, separate from colour
    // Specular transfers information on degree of shininess
    // Specular is independent of texture colours, added separately
    fColor = colour * texture2D( texture, texCoord * texScale) + vec4(specular1/lightDistance + specular2, 1.0);
}
