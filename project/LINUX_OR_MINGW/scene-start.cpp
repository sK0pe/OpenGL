#include "Angel.h"

#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <fstream>	// Part K Added for fstream handling
#include <string>	// Part K Added for C++ string handling
#include <iostream>

// Open Asset Importer header files (in ../../assimp--3.0.1270/include)
// This is a standard open source library for loading meshes, see gnatidread.h
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

GLint windowHeight=640, windowWidth=960;

// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data
// Reader" code.  This file contains parts of the code that you shouldn't need
// to modify (but, you can).
#include "gnatidread.h"
#include "gnatidread2.h"	// ----Part 2 B2----

using namespace std;        // Import the C++ standard functions (e.g., min) 


//  ----Part 2 B3---
//  Added vBoneIDs, vBoneWeights, boneTransformsU
// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram; // The number identifying the GLSL shader program
GLuint vPosition, vNormal, vTexCoord, vBoneIDs, vBoneWeights; // IDs for vshader input vars (from glGetAttribLocation)
GLuint projectionU, modelViewU, uBoneTransforms; // IDs for uniform variables (from glGetUniformLocation)

// ----Part D----
// Scale viewDist by 10 (10*original) (-Z, camera position will be further back)
static float viewDist = 15; // Distance from the camera to the centre of the scene

static float camRotSidewaysDeg=0; // rotates the camera sideways around the centre
static float camRotUpAndOverDeg=20; // rotates the camera up and over the centre.

mat4 projection; // Projection matrix - set in the reshape function
mat4 view; // View matrix - set in the display function.

// These are used to set the window title
char lab[] = "Project1";
char *programName = NULL; // Set in main 
int numDisplayCalls = 0; // Used to calculate the number of frames per second

//------Meshes----------------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                           (numMeshes is defined in gnatidread.h)
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
const aiScene* scenes[numMeshes];	// ----Part 2 B4----
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays

// -----Textures--------------------------------------------------------------
//                           (numTextures is defined in gnatidread.h)
texture* textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures

//------Scene Objects---------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct {
    vec4 loc;
    float scale;
    float angles[3]; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    vec3 rgb;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
} SceneObject;

// ----Part J----
// Minimum number of objects to meet project defaults
// Used by deleteObject() function
const int projectMinObjects = 3;
const int numSaves = 20;
const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0;    // How many objects are currenly in the scene.
int currObject = -1; // The current object
int toolObj = -1;    // The object currently being modified

//----------------------------------------------------------------------------
//
// Loads a texture by number, and binds it for later use.    
void loadTextureIfNotAlreadyLoaded(int i)
{
    if (textures[i] != NULL) return; // The texture is already loaded.

    textures[i] = loadTextureNum(i); CheckError();
    glActiveTexture(GL_TEXTURE0); CheckError();

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]); CheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData); CheckError();
    glGenerateMipmap(GL_TEXTURE_2D); CheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CheckError();

    glBindTexture(GL_TEXTURE_2D, 0); CheckError(); // Back to default texture
}

//------Mesh loading----------------------------------------------------------
//
// The following uses the Open Asset Importer library via loadMesh in 
// gnatidread.h to load models in .x format, including vertex positions, 
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

void loadMeshIfNotAlreadyLoaded(int meshNumber)
{
    if (meshNumber>=numMeshes || meshNumber < 0) {
        printf("Error - no such model number");
        exit(EXIT_FAILURE);
    }

    if (meshes[meshNumber] != NULL)
        return; // Already loaded

    //  ----Part2 B5----
    //  Change how meshes are stored
    const aiScene* scene = loadScene(meshNumber);
    scenes[meshNumber] = scene;
    aiMesh* mesh = scene->mMeshes[0];
    meshes[meshNumber] = mesh;

    glBindVertexArray( vaoIDs[meshNumber] );

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers( 1, buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*(3+3+3)*mesh->mNumVertices,
                  NULL, GL_STATIC_DRAW );

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.    
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(float)*3*nVerts, mesh->mVertices );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*3*nVerts, sizeof(float)*3*nVerts, mesh->mTextureCoords[0] );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);

    // Load the element index data
    GLuint elements[mesh->mNumFaces*3];
    for (GLuint i=0; i < mesh->mNumFaces; i++) {
        elements[i*3] = mesh->mFaces[i].mIndices[0];
        elements[i*3+1] = mesh->mFaces[i].mIndices[1];
        elements[i*3+2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements, GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0                 
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( vPosition );

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer( vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*3*mesh->mNumVertices) );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*6*mesh->mNumVertices) );
    glEnableVertexAttribArray( vNormal );
    CheckError();

    // ----Part 2 B6----
    // Get boneIds and boneWeights for each vertex from the imported mesh data
    //  Initialises th new vertex shader  via gnatiread2.h
    GLint boneIDs[mesh->mNumVertices][4];
    GLfloat boneWeights[mesh->mNumVertices][4];
    getBonesAffectingEachVertex(mesh, boneIDs, boneWeights);

    GLuint buffers[2];
    glGenBuffers( 2, buffers );  // Add two vertex buffer objects
    
    glBindBuffer( GL_ARRAY_BUFFER, buffers[0] ); CheckError();
    glBufferData( GL_ARRAY_BUFFER, sizeof(int)*4*mesh->mNumVertices, boneIDs, GL_STATIC_DRAW ); CheckError();
    glVertexAttribIPointer(vBoneIDs, 4, GL_INT, 0, BUFFER_OFFSET(0)); CheckError();
    glEnableVertexAttribArray(vBoneIDs);     CheckError();
    
    glBindBuffer( GL_ARRAY_BUFFER, buffers[1] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*4*mesh->mNumVertices, boneWeights, GL_STATIC_DRAW );
    glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vBoneWeights);    CheckError();
}

//----------------------------------------------------------------------------

static void mouseClickOrScroll(int button, int state, int x, int y){
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        if (glutGetModifiers() != GLUT_ACTIVE_SHIFT){
            activateTool(button);
        }
        else{
            activateTool(GLUT_MIDDLE_BUTTON);  
      }
    }
    else if (button==GLUT_LEFT_BUTTON && state == GLUT_UP){
        deactivateTool();
    } 
    else if (button==GLUT_MIDDLE_BUTTON && state==GLUT_DOWN) { 
        activateTool(button); 
    }
    else if (button==GLUT_MIDDLE_BUTTON && state==GLUT_UP){
        deactivateTool();  
    }
    else if (button == 3) { // scroll up
        viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05;
    }
    else if (button == 4) { // scroll down
        viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05;
    }
}

//----------------------------------------------------------------------------

static void mousePassiveMotion(int x, int y)
{
    mouseX=x;
    mouseY=y;
}

//----------------------------------------------------------------------------

mat2 camRotZ()
{
    return rotZ(-camRotSidewaysDeg) * mat2(10.0, 0, 0, -10.0);
}

//------callback functions for doRotate below and later-----------------------

static void adjustCamrotsideViewdist(vec2 cv)
{
    cout << cv << endl;
    camRotSidewaysDeg+=cv[0]; viewDist+=cv[1];
}

static void adjustcamSideUp(vec2 su)
{
    camRotSidewaysDeg+=su[0]; camRotUpAndOverDeg+=su[1];
}
    
static void adjustLocXZ(vec2 xz)
{
    sceneObjs[toolObj].loc[0]+=xz[0]; sceneObjs[toolObj].loc[2]+=xz[1];
}

static void adjustScaleY(vec2 sy)
{
    sceneObjs[toolObj].scale+=sy[0]; sceneObjs[toolObj].loc[1]+=sy[1];
}

//----------------------------------------------------------------------------
//------Set the mouse buttons to rotate the camera----------------------------
//------around the centre of the scene.---------------------------------------
//----------------------------------------------------------------------------

static void doRotate(){
    //  ----Part D----
    //  Scale the value of the 4th float by 10, move the camera back along Z
    //  by -20 rather than -2, as Z movement speed needs to be scaled with
    //  changes in Fustrum due to change in nearDist value
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-20),
                     adjustcamSideUp, mat2(400, 0, 0,-90) );
}

//  ----Part J----
//  Function to duplicate an object and set it as current object
//  Will duplicate in place but allow user to move around before setting final
//  position
static void duplicateObject(int objectId){
    if(nObjects < maxObjects+1){   // Check for boundary first
        sceneObjs[nObjects] = sceneObjs[objectId];
        // Set duplicate to current and 
        // Also set it to the tool so that features can be changed
        toolObj = currObject = nObjects++;
        setToolCallbacks(adjustLocXZ, camRotZ(), adjustScaleY, 
            mat2(0.05, 0.0, 0.0, 10.0));
        glutPostRedisplay();    //  Refresh screen
    }
}


//  ----Part J----
//  Function to delete the last generated object
//  Will not delete the minimum specification of objects which includes the
//  project requriements of 2 lights and the ground
static void deleteObject(int objectId){
    if(nObjects > projectMinObjects){
        //  Set current object to previous
        currObject = --nObjects;
        toolObj = -1;
        doRotate();     // Return to camera modification
        glutPostRedisplay();    // Refresh 
    }
}

                                  
//------Add an object to the scene--------------------------------------------

static void addObject(int id)
{

    vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
    sceneObjs[nObjects].loc[0] = currPos[0];
    sceneObjs[nObjects].loc[1] = 0.0;
    sceneObjs[nObjects].loc[2] = currPos[1];
    sceneObjs[nObjects].loc[3] = 1.0;

    if (id!=0 && id!=55)
        sceneObjs[nObjects].scale = 0.005;

    sceneObjs[nObjects].rgb[0] = 0.7; sceneObjs[nObjects].rgb[1] = 0.7;
    sceneObjs[nObjects].rgb[2] = 0.7; sceneObjs[nObjects].brightness = 1.0;

    sceneObjs[nObjects].diffuse = 1.0; sceneObjs[nObjects].specular = 0.5;
    sceneObjs[nObjects].ambient = 0.7; sceneObjs[nObjects].shine = 10.0;

    sceneObjs[nObjects].angles[0] = 0.0; sceneObjs[nObjects].angles[1] = 180.0;
    sceneObjs[nObjects].angles[2] = 0.0;

    sceneObjs[nObjects].meshId = id;
    sceneObjs[nObjects].texId = rand() % numTextures;
    sceneObjs[nObjects].texScale = 2.0;

    toolObj = currObject = nObjects++;
    setToolCallbacks(adjustLocXZ, camRotZ(),
                     adjustScaleY, mat2(0.05, 0, 0, 10.0) );
    glutPostRedisplay();
}

//------The init function-----------------------------------------------------

void init( void )
{
    srand ( time(NULL) ); /* initialize random seed - so the starting scene varies */
    aiInit();

    glGenVertexArrays(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes
    glGenTextures(numTextures, textureIDs); CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    shaderProgram = InitShader( "vStart.glsl", "fStart.glsl" );

    glUseProgram( shaderProgram ); CheckError();

    // Initialize the vertex position attribute from the vertex shader        
    vPosition = glGetAttribLocation( shaderProgram, "vPosition" );
    vNormal = glGetAttribLocation( shaderProgram, "vNormal" );
    // ----Part 2 B3----
    // Initialise additional GLUints
    vBoneIDs = glGetAttribLocation(shaderProgram, "boneIDs");
    vBoneWeights = glGetAttribLocation(shaderProgram, "boneWeights");


    CheckError();

    // Likewise, initialize the vertex texture coordinates attribute.    
    vTexCoord = glGetAttribLocation( shaderProgram, "vTexCoord" );
    CheckError();

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");
    // ----Part 2 B3----
    // Initialise additional GLUints
    uBoneTransforms = glGetUniformLocation(shaderProgram, "boneTransforms");
    CheckError();

    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[0].scale = 10.0;
    sceneObjs[0].angles[0] = 90.0; // Rotate it.
    sceneObjs[0].texScale = 5.0; // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[1].scale = 0.1;
    sceneObjs[1].texId = 0; // Plain texture
    sceneObjs[1].brightness = 0.2; // The light's brightness is 5 times this (below).

    // ----Part I----
    // Object 2 is the second light
    // Add object 2nd light
    addObject(55);  // Sphere for the second light
    sceneObjs[2].loc = vec4(-2.0, 2.0, -2.0, 1.0);
    sceneObjs[2].scale = 0.1;
    sceneObjs[2].texId = 0;
    sceneObjs[2].brightness = 0.2;

    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable( GL_DEPTH_TEST );
    doRotate(); // Start in camera rotate mode.
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}

//----------------------------------------------------------------------------

void drawMesh(SceneObject sceneObj)
{

    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId);
    glActiveTexture(GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

    // Texture 0 is the only texture type in this program, and is for the rgb
    // colour of the surface but there could be separate types for, e.g.,
    // specularity and normals.
    glUniform1i( glGetUniformLocation(shaderProgram, "texture"), 0 );

    // Set the texture scale for the shaders
    glUniform1f( glGetUniformLocation( shaderProgram, "texScale"), sceneObj.texScale );

    // Set the projection matrix for the shaders
    glUniformMatrix4fv( projectionU, 1, GL_TRUE, projection );

    
    // ----Part B----
    // Set the model matrix - this should combine translation, rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).
    // Scale object, then apply rotations Z, Y, X, then translate

    mat4 rotations = RotateX(sceneObj.angles[0]) * RotateY(sceneObj.angles[1]) * RotateZ(sceneObj.angles[2]);
    mat4 model = Translate(sceneObj.loc) * rotations * Scale(sceneObj.scale);


    // Set the model-view matrix for the shaders
    glUniformMatrix4fv( modelViewU, 1, GL_TRUE, view * model );

    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    CheckError();
    glBindVertexArray( vaoIDs[sceneObj.meshId] );
    CheckError();

	// ----Part 2 B7----
	// add code for animation
    int nBones = meshes[sceneObj.meshId]->mNumBones;
    if(nBones == 0)  nBones = 1;  // If no bones, just a single identity matrix is used

    // get boneTransforms for the first (0th) animation at the given time (a float measured in frames)
    //    (Replace <POSE_TIME> appropriately with a float expression giving the time relative to
    //     the start of the animation, measured in frames, like the frame numbers in Blender.)
    mat4 boneTransforms[nBones];     // was: mat4 boneTransforms[mesh->mNumBones];
    calculateAnimPose(meshes[sceneObj.meshId], scenes[sceneObj.meshId], 0, <POSE_TIME>, boneTransforms);
    glUniformMatrix4fv(uBoneTransforms, nBones, GL_Tx`RUE, (const GLfloat *)boneTransforms);



    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3,
                   GL_UNSIGNED_INT, NULL);
    CheckError();
}

//----------------------------------------------------------------------------

void display( void ){
	numDisplayCalls++;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame

    // Set the view matrix. To start with this just moves the camera
    // backwards.  You'll need to add appropriate rotations.

    //  ----Part A----
    //  View Matrix =  Translate(camera's position) * Rotate on X axis * Rotate on Y axis
    view = Translate(0.0, 0.0, -viewDist) * RotateX(camRotUpAndOverDeg) * RotateY(camRotSidewaysDeg);

    //  ----Part I----
    //  Set second light's position
    SceneObject lightObj1 = sceneObjs[1];
    vec4 lightPosition1 = view * lightObj1.loc;
    
    SceneObject lightObj2 = sceneObjs[2];
    vec4 lightPosition2 = view * lightObj2.loc;

    glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition1"), 1, lightPosition1);
    CheckError();
    glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition2"), 1, lightPosition2);
    CheckError();
    //  3 float vector for rgb colour information
    glUniform3fv( glGetUniformLocation(shaderProgram, "lightColour1"), 1, lightObj1.rgb);
    CheckError();
    glUniform3fv( glGetUniformLocation(shaderProgram, "lightColour2"), 1, lightObj2.rgb);
    CheckError();
    //  Single float for brightness
    glUniform1f( glGetUniformLocation(shaderProgram, "lightBrightness1"), lightObj1.brightness);
    CheckError();
    glUniform1f( glGetUniformLocation(shaderProgram, "lightBrightness2"), lightObj2.brightness);
    CheckError();

    for (int i=0; i < nObjects; i++) {
        SceneObject so = sceneObjs[i];

        // ----Part G, Part H----
        // Remove colour from lightObj1 in CPP file as being done in the fstart.glsl
        vec3 rgb = so.rgb * so.brightness * 2.0;
        glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * rgb );
        CheckError();
        glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, so.diffuse * rgb );
        glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, so.specular * rgb );
        glUniform1f( glGetUniformLocation(shaderProgram, "Shininess"), so.shine );
        CheckError();

        drawMesh(sceneObjs[i]);
    }

    glutSwapBuffers();
}

//----------------------------------------------------------------------------
//------Menus-----------------------------------------------------------------
//----------------------------------------------------------------------------

static void objectMenu(int id)
{
    deactivateTool();
    addObject(id);
}

static void texMenu(int id)
{
    deactivateTool();
    if (currObject>=0) {
        sceneObjs[currObject].texId = id;
        glutPostRedisplay();
    }
}

static void groundMenu(int id)
{
    deactivateTool();
    sceneObjs[0].texId = id;
    glutPostRedisplay();
}

static void adjustBrightnessY(vec2 by)
{
    sceneObjs[toolObj].brightness+=by[0];
    sceneObjs[toolObj].loc[1]+=by[1];
}

static void adjustRedGreen(vec2 rg)
{
    sceneObjs[toolObj].rgb[0]+=rg[0];
    sceneObjs[toolObj].rgb[1]+=rg[1];
}

static void adjustBlueBrightness(vec2 bl_br)
{
    sceneObjs[toolObj].rgb[2]+=bl_br[0];
    sceneObjs[toolObj].brightness+=bl_br[1];
}

//  ----Part C----
//  Adjusts the ambience which is the first element of 
//  ambienceDiffusion and the diffusion which is the second
//  element
//  Adds these values, to the scene object.
static void adjustAmbienceDiffusion(vec2 ambienceDiffusion){
    sceneObjs[toolObj].ambient += ambienceDiffusion[0];
    sceneObjs[toolObj].diffuse += ambienceDiffusion[1];
}

//  ----Part C----
//  Adjusts the specularity which is the first element of 
//  specularityShininess and the shininess which is the second
//  element
//  Adds these values, to the scene object.
static void adjustSpecularityShininess(vec2 specularityShininess){
    sceneObjs[toolObj].specular += specularityShininess[0];
    sceneObjs[toolObj].shine += specularityShininess[1];
}


static void lightMenu(int id)
{
    deactivateTool();
    if (id == 70) {
        toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );

    }
    else if (id >= 71 && id <= 74) {
        toolObj = 1;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
    }
    else if(id == 80){
        toolObj = 2;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                            adjustBrightnessY, mat2(1.0, 0.0, 0.0, 10.0));
    }
    else if(id == 81){
        toolObj = 2;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0.0, 0.0, 1.0),
                            adjustBlueBrightness, mat2(1.0, 0.0, 0.0, 1.0));
    }
    else {
        printf("Error in lightMenu\n");
        exit(EXIT_FAILURE);
    }
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int))
{
    int nSubMenus = (size-1)/10 + 1;
    int subMenus[nSubMenus];

    for (int i=0; i < nSubMenus; i++) {
        subMenus[i] = glutCreateMenu(menuFn);
        for (int j = i*10+1; j <= min(i*10+10, size); j++)
            glutAddMenuEntry( menuEntries[j-1] , j);
        CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for (int i=0; i < nSubMenus; i++) {
        char num[6];
        sprintf(num, "%d-%d", i*10+1, min(i*10+10, size));
        glutAddSubMenu(num,subMenus[i]);
        CheckError();
    }
    return menuId;
}


static void materialMenu(int id)
{
    deactivateTool();
    if (currObject < 0) return;
    else toolObj = currObject;

    //  ----Part C----
    //  Menu addition and call to adjust ambience, diffusion, specularity
    //  and shininess of object
    switch(id){
        case 10:
            setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1),
                         adjustBlueBrightness, mat2(1, 0, 0, 1) );
            break;
        case 20:
            setToolCallbacks(adjustAmbienceDiffusion, mat2(1, 0, 0, 1), 
                        adjustSpecularityShininess, mat2(1, 0, 0, 1));
            break;
        default:
            printf("Error in materialMenu\n");
            break;
    }
}

static void adjustAngleYX(vec2 angle_yx){
    sceneObjs[currObject].angles[1]+=angle_yx[0];
    sceneObjs[currObject].angles[0]+=angle_yx[1];
}

static void adjustAngleZTexscale(vec2 az_ts){
    sceneObjs[currObject].angles[2]+=az_ts[0];
    sceneObjs[currObject].texScale+=az_ts[1];
}

static void mainmenu(int id){
    deactivateTool();
    if (id == 41 && currObject>=0) {
        toolObj=currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustScaleY, mat2(0.05, 0, 0, 10) );
    }
    if (id == 50)
        doRotate();
    if (id == 55 && currObject>=0) {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400),
                         adjustAngleZTexscale, mat2(400, 0, 0, 15) );
    }
    if(id == 89 && currObject >= 0){    // ----Part J----
        duplicateObject(currObject);
    }
    if(id == 90 && currObject >= 0){    // ----Part J----
        deleteObject(currObject);
    }
    if (id == 99) exit(EXIT_SUCCESS);
}

// ----Part K----
// Save Scene function
static void saveMenu(int id){
    //  output file stream in binary mode
    string fileName = "Save: " + to_string(id) + ".sav";
    ofstream saveFile(fileName, ios::binary);
    if(saveFile.is_open()){
        saveFile.write((char*)&nObjects, sizeof(int));
        saveFile.write((char*)sceneObjs, sizeof(SceneObject)*nObjects);
        saveFile.write((char*)&viewDist, sizeof(float));
        saveFile.write((char*)&camRotSidewaysDeg, sizeof(float));
        saveFile.write((char*)&camRotUpAndOverDeg, sizeof(float));
        saveFile.close();   // close output file stream
    }
    else{
        cerr << "Could not open file '"<< fileName << "' to write savefile.\n";
    }
}

// ----Part K----
// Load Scene function
static void loadMenu(int id){
    // input file stream in binary mode
    string fileName = "Save: " + to_string(id) + ".sav";
    ifstream saveFile(fileName, ios::binary);
    if(saveFile.is_open()){
        saveFile.read((char*)&nObjects, sizeof(int));
        saveFile.read((char*)sceneObjs, sizeof(SceneObject)*nObjects);
        saveFile.read((char*)&viewDist, sizeof(float));
        saveFile.read((char*)&camRotSidewaysDeg, sizeof(float));
        saveFile.read((char*)&camRotUpAndOverDeg, sizeof(float));
        saveFile.close();  // close input file stream
        doRotate();
    }
    else{
        cerr << "Could not open save file '" << fileName << "'.\n";
    }
}

static void makeMenu()
{
    int objectId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu);

    int materialMenuId = glutCreateMenu(materialMenu);
    glutAddMenuEntry("R/G/B/All",10);
    glutAddMenuEntry("Ambient/Diffuse/Specular/Shine",20);

    int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
    
    int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);

    //  ----Part K----
    //  Creating save and load menus with an array of saves, max length 200 characters
    //  Cannot use ifstream / ofstream as not using C++ handlers
    char saveMenuEntries[numSaves][128];
    for(int s = 0; s < numSaves; ++s){
        sprintf(saveMenuEntries[s], "Save: %d", s+1);
    }
    //  ----Part K----
    int saveMenuId = createArrayMenu(numSaves, saveMenuEntries, saveMenu);

    int loadMenuId = createArrayMenu(numSaves, saveMenuEntries, loadMenu);

    int lightMenuId = glutCreateMenu(lightMenu);
    glutAddMenuEntry("Move Light 1",70);
    glutAddMenuEntry("R/G/B/All Light 1",71);
    glutAddMenuEntry("Move Light 2",80);
    glutAddMenuEntry("R/G/B/All Light 2",81);


    glutCreateMenu(mainmenu);
    glutAddMenuEntry("Rotate/Move Camera",50);
    glutAddSubMenu("Add object", objectId);
    glutAddMenuEntry("Position/Scale", 41);
    glutAddMenuEntry("Rotation/Texture Scale", 55);
    glutAddSubMenu("Material", materialMenuId);
    glutAddSubMenu("Texture",texMenuId);
    glutAddSubMenu("Ground Texture",groundMenuId);
    glutAddSubMenu("Lights",lightMenuId);
    glutAddMenuEntry("Duplicate Last Object", 89);   // Part J
    glutAddMenuEntry("Delete Last Object", 90);
    glutAddSubMenu("Save Scene", saveMenuId);     // Part K
    glutAddSubMenu("Load Scene", loadMenuId);     // Part K
    glutAddMenuEntry("EXIT", 99);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//----------------------------------------------------------------------------

void keyboard( unsigned char key, int x, int y )
{
    switch ( key ) {
    case 033:
        exit( EXIT_SUCCESS );
        break;
    }
}

//----------------------------------------------------------------------------

void idle( void )
{
    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void reshape( int width, int height )
{
    windowWidth = width;
    windowHeight = height;

    glViewport(0, 0, width, height);

    // You'll need to modify this so that the view is similar to that in the
    // sample solution.
    // In particular: 
    //     - the view should include "closer" visible objects (slightly tricky)
    //     - when the width is less than the height, the view should adjust so
    //         that the same part of the scene is visible across the width of
    //         the window.

    //  ----Part D----
    //  Scale the nearDist float by a factor of 0.1, trim less primitives
    //  combined with increasing the viewDist, more will be seen and less
    //  is clipped too early
    GLfloat nearDist = 0.02;

    //  ----Part E----
    //  Determine the parameters to form the Frustum without warping image
    //  on resize when width is lower than height or height is lower than
    //  width
    GLfloat leftBound, rightBound, topBound, bottomBound;

    if(width < height){
        //  If width is less than height, divide height by width
        //  to get a constant scale based on the lower value (width)
        leftBound = -nearDist;
        rightBound = nearDist;
        bottomBound = -nearDist * (float) height / (float) width;
        topBound = nearDist * (float) height / (float) width;
    }
    else{
        //  Reverse of the above situation, the lower value is now
        //  height
        leftBound = -nearDist * (float) width / (float) height;
        rightBound = nearDist * (float) width / (float) height;
        bottomBound = -nearDist;
        topBound = nearDist;
    }
    //  Frustum(left, right, bottom, top, near, far)
    //  near is kept as original as nearDist change affects everything else
    //  far is factoed up by 10
    projection = Frustum(leftBound,
                            rightBound, 
                                bottomBound,
                                    topBound,
                                        0.2,
                                         1000.0); // scaled by 10 for Part D
}

//----------------------------------------------------------------------------

void timer(int unused)
{
    char title[256];
    sprintf(title, "%s %s: %d Frames Per Second @ %d x %d",
                    lab, programName, numDisplayCalls, windowWidth, windowHeight );

    glutSetWindowTitle(title);

    numDisplayCalls = 0;
    glutTimerFunc(1000, timer, 1);
}

//----------------------------------------------------------------------------

char dirDefault1[] = "models-textures";
char dirDefault3[] = "/tmp/models-textures";
char dirDefault4[] = "/d/models-textures";
char dirDefault2[] = "/cslinux/examples/CITS3003/project-files/models-textures";

void fileErr(char* fileName)
{
    printf("Error reading file: %s\n", fileName);
    printf("When not in the CSSE labs, you will need to include the directory containing\n");
    printf("the models on the command line, or put it in the same folder as the exectutable.");
    exit(EXIT_FAILURE);
}

//----------------------------------------------------------------------------

int main( int argc, char* argv[] )
{
    // Get the program name, excluding the directory, for the window title
    programName = argv[0];
    for (char *cpointer = argv[0]; *cpointer != 0; cpointer++)
        if (*cpointer == '/' || *cpointer == '\\') programName = cpointer+1;

    // Set the models-textures directory, via the first argument or some handy defaults.
    if (argc > 1)
        strcpy(dataDir, argv[1]);
    else if (opendir(dirDefault1)) strcpy(dataDir, dirDefault1);
    else if (opendir(dirDefault2)) strcpy(dataDir, dirDefault2);
    else if (opendir(dirDefault3)) strcpy(dataDir, dirDefault3);
    else if (opendir(dirDefault4)) strcpy(dataDir, dirDefault4);
    else fileErr(dirDefault1);

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowSize( windowWidth, windowHeight );

    //glutInitContextVersion( 3, 2);
    glutInitContextProfile( GLUT_CORE_PROFILE );            // May cause issues, sigh, but you
    // glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE ); // should still use only OpenGL 3.2 Core
                                                            // features.
    glutCreateWindow( "Initialising..." );

    glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

    init();
    CheckError();

    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutIdleFunc( idle );

    glutMouseFunc( mouseClickOrScroll );
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc( doToolUpdateXY );
 
    glutReshapeFunc( reshape );
    glutTimerFunc( 1000, timer, 1 );
    CheckError();

    makeMenu();
    CheckError();

    glutMainLoop();
    return 0;
}