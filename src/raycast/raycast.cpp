
// --------------------------------------------------------------------------
// GPU raycasting tutorial
// Made by Peter Trier jan 2007
//
// This file contains all the elements nessesary to implement a simple
// GPU volume raycaster.
// Notice this implementation requires a shader model 3.0 gfxcard
// --------------------------------------------------------------------------

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cassert>
#include "Vector3.h"
#include "controls.hpp"
#include <string>
#include "common/perlin.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define WINDOW_SIZE 800
#define VOLUME_TEX_SIZE 128

using namespace std;

// Globals ------------------------------------------------------------------


bool toggle_visuals = true;
CGcontext context;
CGprofile vertexProfile, fragmentProfile;
CGparameter param1,param2;
GLuint renderbuffer;
GLuint framebuffer;
CGprogram vertex_main,fragment_main; // the raycasting shader programs
GLuint volume_texture; // the volume texture
GLuint backface_buffer; // the FBO buffers
GLuint final_image;
float stepsize = 1.0/50.0;

/// Implementation ----------------------------------------


void cgErrorCallback()
{
	CGerror lastError = cgGetError();
	if(lastError)
	{
		cout << cgGetErrorString(lastError) << endl;
		if(context != NULL)
			cout << cgGetLastListing(context) << endl;
		exit(0);
	}
}

// Sets a uniform texture parameter
void set_tex_param(string par, GLuint tex,const CGprogram &program,CGparameter param)
{
	param = cgGetNamedParameter(program, par.c_str());
	cgGLSetTextureParameter(param, tex);
	cgGLEnableTextureParameter(param);
}


// load_vertex_program: loading a vertex program
void load_vertex_program(CGprogram &v_program, string shader_path, string program_name)
{
	assert(cgIsContext(context));
	v_program = cgCreateProgramFromFile(context, CG_SOURCE,shader_path.c_str(),
		vertexProfile,program_name.c_str(), NULL);
	if (!cgIsProgramCompiled(v_program))
		cgCompileProgram(v_program);

	cgGLEnableProfile(vertexProfile);
	cgGLLoadProgram(v_program);
	cgGLDisableProfile(vertexProfile);
}

// load_fragment_program: loading a fragment program
void load_fragment_program(CGprogram &f_program,string shader_path, string program_name)
{
	assert(cgIsContext(context));
	f_program = cgCreateProgramFromFile(context, CG_SOURCE, shader_path.c_str(),
		fragmentProfile,program_name.c_str(), NULL);
	if (!cgIsProgramCompiled(f_program))
		cgCompileProgram(f_program);

	cgGLEnableProfile(fragmentProfile);
	cgGLLoadProgram(f_program);
	cgGLDisableProfile(fragmentProfile);
}

void enable_renderbuffers()
{
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, framebuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
}

void disable_renderbuffers()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void vertex(float x, float y, float z)
{
	glColor3f(x,y,z);
	glMultiTexCoord3fARB(GL_TEXTURE1_ARB, x, y, z);
	glVertex3f(x,y,z);
}
// this method is used to draw the front and backside of the volume
void drawQuads(float x, float y, float z)
{

	glBegin(GL_QUADS);
	/* Back side */
	glNormal3f(0.0, 0.0, -1.0);
	vertex(0.0, 0.0, 0.0);
	vertex(0.0, y, 0.0);
	vertex(x, y, 0.0);
	vertex(x, 0.0, 0.0);

	/* Front side */
	glNormal3f(0.0, 0.0, 1.0);
	vertex(0.0, 0.0, z);
	vertex(x, 0.0, z);
	vertex(x, y, z);
	vertex(0.0, y, z);

	/* Top side */
	glNormal3f(0.0, 1.0, 0.0);
	vertex(0.0, y, 0.0);
	vertex(0.0, y, z);
    vertex(x, y, z);
	vertex(x, y, 0.0);

	/* Bottom side */
	glNormal3f(0.0, -1.0, 0.0);
	vertex(0.0, 0.0, 0.0);
	vertex(x, 0.0, 0.0);
	vertex(x, 0.0, z);
	vertex(0.0, 0.0, z);

	/* Left side */
	glNormal3f(-1.0, 0.0, 0.0);
	vertex(0.0, 0.0, 0.0);
	vertex(0.0, 0.0, z);
	vertex(0.0, y, z);
	vertex(0.0, y, 0.0);

	/* Right side */
	glNormal3f(1.0, 0.0, 0.0);
	vertex(x, 0.0, 0.0);
	vertex(x, y, 0.0);
	vertex(x, y, z);
	vertex(x, 0.0, z);
	glEnd();

}

float gw4DNoise(float x, float y, float z,
				float frequency, float offset, float freqMult, float roughness, float octaves)
{
	int i;
	float value = 0;
	float remainder;

	for (i = 0; (float)i < octaves; i++){
		if (i==0){
			value += noise::PerlinNoise3D(x*frequency+offset, y*frequency+offset, z*frequency+offset, 5,6,3)-0.5;
		} else {
			value += (noise::PerlinNoise3D(x*frequency+offset, y*frequency+offset, z*frequency+offset, 5,6,3)-0.5 ) * pow(roughness, (float)i);
		}
		frequency = frequency * freqMult;
		offset = offset * freqMult;
		}

	remainder = octaves - floor(octaves);

	if (octaves > 0)
		{
		value += remainder * (noise::PerlinNoise3D(x*frequency+offset, y*frequency+offset, z*frequency+offset, 5,6,3)-0.5 ) * pow(roughness, (float)i);

		}

	return value;

}

// create a test volume texture, here you could load your own volume
void create_volumetexture()
{
	cout << "generating volume texture"<<endl;
	srand ( time(NULL) );

	int size = VOLUME_TEX_SIZE*VOLUME_TEX_SIZE*VOLUME_TEX_SIZE* 4;
	//GLubyte *data = new GLubyte[size];

	auto n = VOLUME_TEX_SIZE;

	float r =0.025f;

	//GLubyte *ptr = data;

	float rnd = (((float)rand())/RAND_MAX)/3;
	int offset1 = rand()%50;
	int offset2 = rand()%50;
	int offset3 = rand()%50;

	unsigned char *data = new unsigned char[n*n*n];
	unsigned char *ptr = data;
	float frequency = 3.0f / n;
	float center = n / 2.0f + 0.5f;

	int progress = 0;
	int lastpercent = -1;
	int total = n*n*n;

	for(int x=0; x < n; ++x) {
		for (int y=0; y < n; ++y) {
			for (int z=0; z < n; ++z) {

				float dx = center-x;
				float dy = center-y;
				float dz = center-z;

				float off = 1.0;//gw4DNoise(x,y,z, frequency, 0 ,1.1+rnd, 1.1+rnd, 4);
				off = abs(off);
				//off = 1-pow(off/2,2);
				//cout<<off<<endl;
				float off1 = fabsf((float) noise::PerlinNoise3D(
					x*frequency+offset1,
					y*frequency+offset2,
					z*frequency+offset3,
					5,
					6, 3));
				off *= off1;
				off = pow(off, 0.1);
				float d = sqrtf(dx*dx+dy*dy+dz*dz)/(n);
				//cout << d <<endl;
				bool isFilled = (d-off1) < r;
				*ptr++ = isFilled ? off*255 : 0;
				//*ptr++ = (int)(off*255);

				progress += 1;
				int percent = (int)(100*((float)progress/total));
				if (percent!=lastpercent){
					lastpercent = percent;
					cout << "progress: " << percent <<"%" <<endl;
				}
			}
		}
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glGenTextures(1, &volume_texture);
	glBindTexture(GL_TEXTURE_3D, volume_texture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	//glTexImage3D(GL_TEXTURE_3D, 0,GL_RGBA, VOLUME_TEX_SIZE, VOLUME_TEX_SIZE,VOLUME_TEX_SIZE,0, GL_RGBA, GL_UNSIGNED_BYTE,data);
	glTexImage3D(GL_TEXTURE_3D, 0,
			     GL_LUMINANCE,
			     n, n, n, 0,
			     GL_LUMINANCE,
			     GL_UNSIGNED_BYTE,
			     data);

	delete []data;
	cout << "volume texture generated" << endl;

}

void init()
{
	cout << "initializing glew" << endl;
	GLenum err = glewInit();

	// initialize all the OpenGL extensions
	glewGetExtension("glMultiTexCoord2fvARB");
	if(glewGetExtension("GL_EXT_framebuffer_object"))	cout << "GL_EXT_framebuffer_object supported"   << endl;
	if(glewGetExtension("GL_EXT_renderbuffer_object"))	cout << "GL_EXT_renderbuffer_object supported"  << endl;
	if(glewGetExtension("GL_ARB_vertex_buffer_object")) cout << "GL_ARB_vertex_buffer_object supported" << endl;
	if(GL_ARB_multitexture)cout << "GL_ARB_multitexture supported " << endl;

	if (glewGetExtension("GL_ARB_fragment_shader")      != GL_TRUE ||
		glewGetExtension("GL_ARB_vertex_shader")        != GL_TRUE ||
		glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
		glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)
	{
		cout << "driver does not support OpenGL Shading Language" << endl;
		exit(1);
	}

	glEnable(GL_CULL_FACE);
	glClearColor(0.0, 0.0, 0.0, 0);
	create_volumetexture();

	cout << "initializing Cg" << endl;
	cgSetErrorCallback(cgErrorCallback);
	context = cgCreateContext();
	if (cgGLIsProfileSupported(CG_PROFILE_VP40)){
		vertexProfile = CG_PROFILE_VP40;
		cout << "CG_PROFILE_VP40 supported." << endl;
	} else {
		if (cgGLIsProfileSupported(CG_PROFILE_ARBVP1))
			vertexProfile = CG_PROFILE_ARBVP1;
		else {
			cout << "Neither arbvp1 or vp40 vertex profiles supported on this system." << endl;
			exit(1);
		}
	}

	if (cgGLIsProfileSupported(CG_PROFILE_FP40)) {
		fragmentProfile = CG_PROFILE_FP40;
		cout << "CG_PROFILE_FP40 supported." << endl;
	} else {
		if (cgGLIsProfileSupported(CG_PROFILE_ARBFP1))
			fragmentProfile = CG_PROFILE_ARBFP1;
		else {
			cout << "Neither arbfp1 or fp40 fragment profiles supported on this system." << endl;
			exit(1);
		}
	}

	cout << "loading vertex shader"<<endl;
	load_vertex_program(vertex_main,"shader.cg","vertex_main");
	cgErrorCallback();

	cout << "loading fragment shader"<<endl;
	load_fragment_program(fragment_main,"shader.cg","fragment_main");
	cgErrorCallback();

	// Create the to FBO's one for the backside of the volumecube and one for the finalimage rendering
	cout << "creating 'backside' and 'volume' framebuffers" << endl;
	glGenFramebuffersEXT(1, &framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,framebuffer);

	glGenTextures(1, &backface_buffer);
	glBindTexture(GL_TEXTURE_2D, backface_buffer);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, WINDOW_SIZE, WINDOW_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, backface_buffer, 0);

	glGenTextures(1, &final_image);
	glBindTexture(GL_TEXTURE_2D, final_image);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, WINDOW_SIZE, WINDOW_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);

	glGenRenderbuffersEXT(1, &renderbuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, WINDOW_SIZE, WINDOW_SIZE);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
}

// glut idle function
void idle_func()
{
	controls::idle();
	glutPostRedisplay();
}

void reshape_ortho(int w, int h)
{
	if (h == 0) h = 1;
	glViewport(0, 0,w,h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
}


void resize(int w, int h)
{
	if (h == 0) h = 1;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)w/(GLfloat)h, 0.01, 400.0);
	glMatrixMode(GL_MODELVIEW);
}

void draw_fullscreen_quad()
{
	glDisable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);

	glTexCoord2f(0,0);
	glVertex2f(0,0);

	glTexCoord2f(1,0);
	glVertex2f(1,0);

	glTexCoord2f(1, 1);

	glVertex2f(1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);

	glEnd();
	glEnable(GL_DEPTH_TEST);

}

// display the final image on the screen
void render_buffer_to_screen()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	if(toggle_visuals)
		glBindTexture(GL_TEXTURE_2D,final_image);
	else
		glBindTexture(GL_TEXTURE_2D,backface_buffer);
	reshape_ortho(WINDOW_SIZE,WINDOW_SIZE);
	draw_fullscreen_quad();
	glDisable(GL_TEXTURE_2D);
}

// render the backface to the offscreen buffer backface_buffer
void render_backface()
{
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, backface_buffer, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	drawQuads(1.0,1.0, 1.0);
	glDisable(GL_CULL_FACE);
}

void raycasting_pass()
{
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, final_image, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	cgGLEnableProfile(vertexProfile);
	cgGLEnableProfile(fragmentProfile);
	cgGLBindProgram(vertex_main);
	cgGLBindProgram(fragment_main);
	cgGLSetParameter1f( cgGetNamedParameter( fragment_main, "stepsize") , stepsize);
	set_tex_param("tex",backface_buffer,fragment_main,param1);
	set_tex_param("volume_tex",volume_texture,fragment_main,param2);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	drawQuads(1.0,1.0, 1.0);
	glDisable(GL_CULL_FACE);
	cgGLDisableProfile(vertexProfile);
	cgGLDisableProfile(fragmentProfile);
}

bool rot_mode = false;
int rot_x = 0;
int rot_y = 0;
float rot_h = 0;
float rot_v = 0;
float rot_h_vel = 0;
float rot_v_vel = 0;
float rot_accel = 0.5;
float rot_drag = 0.95;


bool pan_mode = false;
int pan_x = 0;
int pan_y = 0;
float pan_vel = 0;
float pan_accel = 0.005;
float pan_drag = 0.95;
float _xdistance = 0.1;


// This display function is called once pr frame
void display()
{
	//static float rotate = 0;
	//rotate += 0.25;
	controls::enterFrame();

	resize(WINDOW_SIZE,WINDOW_SIZE);
	enable_renderbuffers();

	glLoadIdentity();
	glTranslatef(0,0,-2.25);
	glTranslatef(0,0,_xdistance);
	glRotatef(rot_h,0,1,0);
	glRotatef(rot_v, cos(rot_h * M_PI/180), 0, sin(rot_h*M_PI/180));
	glTranslatef(-0.5,-0.5,-0.5);
	render_backface();
	raycasting_pass();
	disable_renderbuffers();
	render_buffer_to_screen();
	glutSwapBuffers();
}


void setupControls(){

	controls::init();
	controls::onKeyDown('w', [](){
		stepsize += 1.0/2048.0;
		if(stepsize > 0.5) stepsize = 0.5 ;
	});

	controls::onKeyDown('e', [](){
		stepsize -= 1.0/2048.0;
		if(stepsize <= 1.0/200.0) stepsize = 1.0/200.0;
	});

	controls::onKeyRelease(' ', [](){
		toggle_visuals = !toggle_visuals;
	});

	controls::onMousePress([](int button, int x, int y){
		if (button == GLUT_LEFT_BUTTON){
			rot_mode = true;
			rot_x = x;
			rot_y = y;
		} else if (button == GLUT_RIGHT_BUTTON){
			pan_mode = true;
			pan_x = x;
			pan_y = y;
		}
	});

	controls::onMouseRelease([](int button, int x, int y){
		if (button == GLUT_LEFT_BUTTON){
			rot_mode = false;
		} else if (button == GLUT_RIGHT_BUTTON){
			pan_mode = false;
		}
	});

	controls::onEnterFrame([](){
		int x = controls::mouse_x;
		int y = controls::mouse_y;
		if(rot_mode){
			rot_h_vel = (x-rot_x)*rot_accel;
			rot_h += rot_h_vel;
			rot_x = x;

			rot_v_vel = (y-rot_y)*rot_accel;
			rot_v += rot_v_vel;
			rot_y = y;
		} else {
			rot_h += rot_h_vel;
			rot_h_vel *= rot_drag;
			rot_v += rot_v_vel;
			rot_v_vel *= rot_drag;
		}

		if(pan_mode){
			pan_vel = ((float)(x-pan_x-y+pan_y))*pan_accel;
			_xdistance += pan_vel;
			pan_x = x;
			pan_y = y;
		} else {
			_xdistance += pan_vel;
			pan_vel *= pan_drag;
		}
	});

}

int main(int argc, char* argv[])
{
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("super duper raycasting");
	glutReshapeWindow(WINDOW_SIZE,WINDOW_SIZE);

	glutDisplayFunc(display);
	glutIdleFunc(idle_func);
	glutReshapeFunc(resize);
	resize(WINDOW_SIZE,WINDOW_SIZE);

	setupControls();
	init();


	glutMainLoop();

	return 0;
}
