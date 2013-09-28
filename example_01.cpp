
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>
#include <algorithm>
#include <png.h>



#define PI 3.14159265  // Should be used from mathlib
inline float sqr(float x) { return x*x; }

using namespace std;

//****************************************************
// Some Classes
//****************************************************

class Viewport;

class Viewport {
  public:
    int w, h; // width and height
};


//****************************************************
// Global Variables
//****************************************************
Viewport	viewport
;



//****************************************************
// Simple init function
//****************************************************
void initScene(){

  // Nothing to do here for this simple example.

}


//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
  viewport.w = w;
  viewport.h = h;

  glViewport (0,0,viewport.w,viewport.h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, viewport.w, 0, viewport.h)
;
}


//****************************************************
// A routine to set a pixel by drawing a GL point.  This is not a
// general purpose routine as it assumes a lot of stuff specific to
// this example.
//****************************************************

void setPixel(int x, int y, GLfloat r, GLfloat g, GLfloat b) {
  glColor3f(r, g, b);
  glVertex2f(x + 0.5, y + 0.5);   // The 0.5 is to target pixel
  // centers 
  // Note: Need to check for gap
  // bug on inst machines.
}

/**
 * Our shit */

/** 
 * Combine and cap RGB values to find resultant RGB
 * PARAM rgbs : array of the RGBs
 * PARAM num : length of rgbs array (# of RGBs to be combined) */
vector<float> shAverager(vector<float> rgbs[], int num) {
  vector<float> result(3, 0.0f);
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  int i;
  for (i = 0; i < num; i++) {
    r = r + rgbs[i].at(0);
    g = g + rgbs[i].at(1);
    b = b + rgbs[i].at(2);
  }
  // Cap when overflow
  if (r > 1.0f)
    r = 1.0f;
  if (g > 1.0f)
    g = 1.0f;
  if (b > 1.0f)
    b = 1.0f;
  result.at(0) = r;
  result.at(1) = g;
  result.at(2) = b;
  return result;
}


// Normal finder (normalize as well)
vector<float> normalizer(vector<float> spherePt) {
  float mag = sqrt(pow(spherePt.at(0), 2.0) + pow(spherePt.at(1), 2.0) + 
		   pow(spherePt.at(2), 2.0));
  vector<float> result(3, 0.0f);
  result.at(0) = spherePt.at(0)/mag;
  result.at(1) = spherePt.at(1)/mag;
  result.at(2) = spherePt.at(2)/mag;
  return result;
}

// Vector finder (subtracting two points) (normalize this as well)

vector<float> vectorizer(vector<float> pt1, vector<float> pt2) { // going from pt1 to pt2
  vector<float> result (3, 0.0f);
  result.at(0) = pt2.at(0)-pt1.at(0);
  result.at(1) = pt2.at(1)-pt1.at(1);
  result.at(2) = pt2.at(2)-pt1.at(2);
  return normalizer(result);
}

vector<float> getRefRay(vector<float> lightDir, vector<float> surfNorm){
  float dot = lightDir.at(0)*surfNorm.at(0) + lightDir.at(1)*surfNorm.at(1) + lightDir.at(2)*surfNorm.at(2);
  vector<float> result(3, 0.0f);
  result.at(0) = -lightDir.at(0) + 2*dot*surfNorm.at(0);
  result.at(1) = -lightDir.at(1) + 2*dot*surfNorm.at(1);
  result.at(2) = -lightDir.at(2) + 2*dot*surfNorm.at(2);
  return normalizer(result);
}

// Ambient function
vector<float> ambientify(vector<float> acolor, vector<float> icolor) {
  vector<float> result(3, 0.0f);
  result.at(0) = acolor.at(0) * icolor.at(0);
  result.at(1) = acolor.at(1) * icolor.at(1);
  result.at(2) = acolor.at(2) * icolor.at(2);
  return result;
}

/** 
 * Diffuse function
 * returns Rd, diffuse RGB value */
vector<float> diffusify(vector<float> dcolor, vector<float> icolor, vector<float> lightDir, vector<float> surfaceNormal)  { // make sure vectors are normalized
  // negative scalar due to the way we are interpreting our light rays
  float scalar = max(-lightDir.at(0)*surfaceNormal.at(0) +
		     -lightDir.at(1)*surfaceNormal.at(1) + 
		     -lightDir.at(2)*surfaceNormal.at(2),
		     0.0f);
  vector<float> result(3, 0.0f);
  result.at(0) = scalar*dcolor.at(0)* icolor.at(0);
  result.at(1) = scalar*dcolor.at(1)* icolor.at(1);
  result.at(2) = scalar*dcolor.at(2)* icolor.at(2);
  return result;
}

/** 
 * Specular function
 * reflectedRay found using PARAMS lightDir and PARAMS surfaceNorm
 * input vectors MUST be normalized first
 * returns Rs, specular RGB value */
vector<float> specularify(vector<float> scolor, vector<float> icolor, 
			  vector<float> lightDir, vector<float> surfaceNorm, 
			  vector<float> dirToViewer, float pCoeff)  { 
  
  // find reflectedRay
  vector<float> reflectRay(3, 0.0f);
  reflectRay = getRefRay(lightDir, surfaceNorm); // getRefRay already normalizes.

  float scalar = max(-reflectRay.at(0)*dirToViewer.at(0) + 
		     -reflectRay.at(1)*dirToViewer.at(1) + 
		     -reflectRay.at(2)*dirToViewer.at(2),
		     0.0f);
  scalar = pow(scalar, pCoeff);

  vector<float> result(3, 0.0f);
  result.at(0) = scalar*scolor.at(0)* icolor.at(0);
  result.at(1) = scalar*scolor.at(1)* icolor.at(1);
  result.at(2) = scalar*scolor.at(2)* icolor.at(2);
  return result;
}

/**
 *  Anisotropic mapping setup */
map<float, float> anisMapX;
map<float, float> anisMapY;

/**
 *  Anisotropic mapper
 *  PARAMS original RGB (pre-Capped) and PARAMS y
 *  to return a resultant anisotropic mapped RGB */
vector<float> anisotropify(vector<float> rgb, float x, float y) {
  vector<float> result(3, 0.0f);
  float scale;
  // if 'y' already exists in anisMap, assign 'y' to scale.
  if (anisMapY.count(y)) {
    map<float, float>::iterator iter = anisMapY.find(y);
    scale = iter->second;
  } else {
    scale = (float) rand()/ double(RAND_MAX);
    scale = 1.0f - 0.1f*scale;
    anisMapY[y] = scale;
  }
  result.at(0) = rgb.at(0)*scale;
  result.at(1) = rgb.at(1)*scale;
  result.at(2) = rgb.at(2)*scale;
  return result;
}


// Classes needed for storing info 
class Color;
class PowerCoeff;
class PointLight;
class DirectionalLight;

class Color {
  public:
    static bool aExists;
    static bool dExists;
    static bool sExists;
    vector<float> rgb;
    Color() : rgb(3, 0.0f) {}
};

bool Color::aExists = false;
bool Color::dExists = false;
bool Color::sExists = false;

class PowerCoeff {
  public:
    static bool isExists;
    float v;
};

bool PowerCoeff::isExists = false;

class PointLight {
  public:
    vector<float> pl;
    vector<float> rgb;
    static int count;
    PointLight() : pl(3, 0.0f), rgb(3, 0.0f) {}
};
int PointLight::count = 0;

class DirectionalLight {
  public:
    vector<float> dl;
    vector<float> rgb;
    static int count;
    DirectionalLight() : dl(3, 0.0f), rgb(3, 0.0f) {}
};
int DirectionalLight::count = 0;

PowerCoeff pc;
PointLight plArray[5];
DirectionalLight dlArray[5];
Color ka, kd, ks;

// OPTIONAL E.C. FEATURES
int anisTrue;
int toonTrue;
int pngTrue;







//****************************************************
// Draw a filled circle.  
//****************************************************

void circle(float centerX, float centerY, float radius) {
  // Draw inner circle
  glBegin(GL_POINTS);
  // We could eliminate wasted work by only looping over the pixels
  // inside the sphere's radius.  But the example is more clear this
  // way.  In general drawing an object by loopig over the whole
  // screen is wasteful.

  int i,j;  // Pixel indices

  int minI = max(0,(int)floor(centerX-radius));
  int maxI = min(viewport.w-1,(int)ceil(centerX+radius));

  int minJ = max(0,(int)floor(centerY-radius));
  int maxJ = min(viewport.h-1,(int)ceil(centerY+radius));

  for (i=0;i<viewport.w;i++) {
    for (j=0;j<viewport.h;j++) {
      // Location of the center of pixel relative to center of sphere
      float x = (i+0.5-centerX);
      float y = (j+0.5-centerY);
      float dist = sqrt(sqr(x) + sqr(y));
      
      if (dist<=radius) {

        // This is the front-facing Z coordinate
        float z = sqrt(radius*radius-dist*dist);
	
	///////////
        // SETUP //
        ///////////
	
	// XYZ vector
	vector<float> xyz(3, 0.0f);
	xyz.at(0) = x;
	xyz.at(1) = y;
	xyz.at(2) = z;

	// SURFACENORMAL vector
	vector<float> sn(3, 0.0f);
	sn.at(0) = x;
	sn.at(1) = y;
	sn.at(2) = z;
	sn = normalizer(sn);

	// VIEWER direction vector
	vector<float> dirToView(3, 0.0f);
	dirToView.at(0) = 0.0f;
	dirToView.at(1) = 0.0f;
	dirToView.at(2) = 1.0f;
	dirToView = normalizer(dirToView);

	///////////
        // BEGIN //
        ///////////
	
	// allOfTheLights: array of the component RGB vectors from each light
	vector<float> aotl[PointLight::count + DirectionalLight::count];

	// PointLight calculations
	for (int k = 0; k < PointLight::count; k++) {
	  // First form array, used to pass through Averager
	  int arraySize = 0;
	  if (Color::aExists)
	    arraySize++;
	  if (Color::dExists)
	    arraySize++;
	  if (Color::sExists)
	    arraySize++;
	  vector<float> rgbs[arraySize];
	  // Run through ka, kd, ks, to find component RGBs
	  int currentPos = 0;
	  // ka
	  if (Color::aExists) {
	    vector<float> aRGB(3, 0.0f);
	    // Ambientify
	    aRGB = ambientify(ka.rgb, plArray[k].rgb);
	    rgbs[currentPos] = aRGB;
	    currentPos++;
	  }
	  // kd
	  if (Color::dExists) {
	    vector<float> dRGB(3, 0.0f);
	    vector<float> lightdir(3, 0.0f);
	    vector<float> plScaled(3, 0.0f);
	    // Find the light direction using vectorizer (using plScaled)
	    plScaled.at(0) = plArray[k].pl.at(0)*radius;
	    plScaled.at(1) = plArray[k].pl.at(1)*radius;
	    plScaled.at(2) = plArray[k].pl.at(2)*radius;
	    lightdir = vectorizer(plScaled, xyz);
	    // Diffusify
	    dRGB = diffusify(kd.rgb, plArray[k].rgb, lightdir, sn);
	    rgbs[currentPos] = dRGB;
	    currentPos++;
	  }
	  // ks
	  if (Color::sExists) {
	    vector<float> sRGB(3, 0.0f);
	    vector<float> lightdir(3, 0.0f);
	    vector<float> plScaled(3, 0.0f);
	    plScaled.at(0) = plArray[k].pl.at(0)*radius;
	    plScaled.at(1) = plArray[k].pl.at(1)*radius;
	    plScaled.at(2) = plArray[k].pl.at(2)*radius;
	    // Find the light direction using vectorizer (using plScaled)
	    lightdir = vectorizer(plScaled, xyz);
	    sRGB = specularify(ks.rgb, plArray[k].rgb, lightdir, sn, dirToView, pc.v);
	    rgbs[currentPos] = sRGB;
	  }
	  // Combine, then insert into aotl
	  aotl[k] = shAverager(rgbs, arraySize);
	}

	// DirectionalLight calculations
	for (int k = 0; k < DirectionalLight::count; k++) {
	  // First form array, so as to pass through Averager
	  int arraySize = 0;
	  if (Color::aExists)
	    arraySize++;
	  if (Color::dExists)
	    arraySize++;
	  if (Color::sExists)
	    arraySize++;
	  vector<float> rgbs[arraySize];
	  // Run through ka, kd, ks, to find component RGBs
	  int currentPos = 0;
	  // ka
	  if (Color::aExists) {
	    vector<float> aRGB(3, 0.0f);
	    // Ambientify
	    aRGB = ambientify(ka.rgb, dlArray[k].rgb);
	    if (anisTrue)
	      aRGB = anisotropify(aRGB, x, y);
	    rgbs[currentPos] = aRGB;
	    currentPos++;
	  }
	  // kd
	  if (Color::dExists) {
	    vector<float> dRGB(3, 0.0f);
	    vector<float> lightdir(3, 0.0f);
	    // Normalize the directional light
	    lightdir = normalizer(dlArray[k].dl);
	    // Diffusify
	    dRGB = diffusify(kd.rgb, dlArray[k].rgb, lightdir, sn);
	    if (anisTrue)
	      dRGB = anisotropify(dRGB, x, y);
	    rgbs[currentPos] = dRGB;
	    currentPos++;
	  }
	  // ks
	  if (Color::sExists) {
	    vector<float> sRGB(3, 0.0f);
	    vector<float> lightdir(3, 0.0f);
	    // Normalize the directional light
	    lightdir = normalizer(dlArray[k].dl);
	    sRGB = specularify(ks.rgb, dlArray[k].rgb, lightdir, sn, dirToView, pc.v);
	    if (anisTrue)
	      sRGB = anisotropify(sRGB, x, y);
	    rgbs[currentPos] = sRGB;
	  }
	  // Combine, then insert into aotl
	  aotl[k + PointLight::count] = shAverager(rgbs, arraySize);
	}

	if (anisTrue) {
	  for (int k = 0; k < PointLight::count + DirectionalLight::count; k++)
	    aotl[k] = anisotropify(aotl[k], x, y);
	}

	// Result RGB for given pixel
	vector<float> result(3, 0.0f);
	// Set result to Combined of allOfTheLights
	result = shAverager(aotl, PointLight::count + DirectionalLight::count);

	// TOON SHADING
	if (toonTrue) {
	  for (int k = 0; k < 3; k++) {
	    if (result.at(k) >= 0.0f && result.at(k) < 0.1f) {
	      result.at(k) = 0.0f;
	    } else if (result.at(k) < 0.2f) {
	      result.at(k) = 0.15f;
	    } else if (result.at(k) < 0.4f) {
	      result.at(k) = 0.32f;
	    } else if (result.at(k) < 0.8f) {
	      result.at(k) = 0.57f;
	    } else if (result.at(k) < 0.9f) {
	      result.at(k) = 0.85f;
	    } else {
	      result.at(k) = 1.0f;
	    }
	  }
	}

	setPixel(i,j, result.at(0), result.at(1), result.at(2));


        // This is amusing, but it assumes negative color values are treated reasonably.
        //setPixel(i,j, x/radius, y/radius, z/radius );
      }

    }
  }


  glEnd();
}
//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

  glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

  glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
  glLoadIdentity();				        // make sure transformation is "zero'd"


  // Start drawing

  circle(viewport.w / 2.0 , viewport.h / 2.0 , min(viewport.w, viewport.h) / 3.0);
  
  glFlush();
  glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}

/** 
 * spacebar, to implement spacebar functionality */
void spacebar(unsigned char key, int x, int y) {
  if (key == 32)
    exit(0);
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {
  // Parse the command-line args and create appropriate objects
  for (int i = 1; i < argc;) {
    if (string(argv[i]) == "-ka") {
      Color::aExists = true;
      ka.rgb.at(0) = atof(argv[i+1]);
      ka.rgb.at(1) = atof(argv[i+2]);
      ka.rgb.at(2) = atof(argv[i+3]);
      i += 4;
      continue;
    }
    if (string(argv[i]) == "-kd") {
      Color::dExists = true;
      kd.rgb.at(0) = atof(argv[i+1]);
      kd.rgb.at(1) = atof(argv[i+2]);
      kd.rgb.at(2) = atof(argv[i+3]);
      i += 4;
      continue;
    }
    if (string(argv[i]) == "-ks") {
      Color::sExists = true;
      ks.rgb.at(0) = atof(argv[i+1]);
      ks.rgb.at(1) = atof(argv[i+2]);
      ks.rgb.at(2) = atof(argv[i+3]);
      i += 4;
      continue;
    }
    if (string(argv[i]) == "-sp" ) {
      PowerCoeff::isExists = true;
      pc.v = atof(argv[i+1]);
      i += 2;
      continue;
    }
    if (string(argv[i]) == "-pl") {
      plArray[PointLight::count].pl.at(0) = atof(argv[i+1]);
      plArray[PointLight::count].pl.at(1) = atof(argv[i+2]);
      plArray[PointLight::count].pl.at(2) = atof(argv[i+3]);
      plArray[PointLight::count].rgb.at(0) = atof(argv[i+4]);
      plArray[PointLight::count].rgb.at(1) = atof(argv[i+5]);
      plArray[PointLight::count].rgb.at(2) = atof(argv[i+6]);
      PointLight::count++;
      i += 7;
      continue;
    }
    if (string(argv[i]) == "-dl") {
      dlArray[DirectionalLight::count].dl.at(0) = atof(argv[i+1]);
      dlArray[DirectionalLight::count].dl.at(1) = atof(argv[i+2]);
      dlArray[DirectionalLight::count].dl.at(2) = atof(argv[i+3]);
      dlArray[DirectionalLight::count].rgb.at(0) = atof(argv[i+4]);
      dlArray[DirectionalLight::count].rgb.at(1) = atof(argv[i+5]);
      dlArray[DirectionalLight::count].rgb.at(2) = atof(argv[i+6]);
      DirectionalLight::count++;
      i += 7;
      continue;
    }

    // OPTIONAL E.C. FEATURES
    if (string(argv[i]) == "-an") {
      anisTrue = 1;
      i += 1;
      break;
    }
    if (string(argv[i]) == "-to") {
      toonTrue = 1;
      i += 1;
      break;
    }
    if (string(argv[i]) == "-pn") {
      pngTrue = 1;
      i += 1;
      break;
    }
  }









  
  //This initializes glut
  glutInit(&argc, argv);

  //This tells glut to use a double-buffered window with red, green, and blue channels 
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  // Initalize theviewport size
  viewport.w = 400;
  viewport.h = 400;

  //The size and position of the window
  glutInitWindowSize(viewport.w, viewport.h);
  glutInitWindowPosition(0,0);
  glutCreateWindow(argv[0]);

  initScene();							// quick function to set up scene

  glutDisplayFunc(myDisplay);				// function to run when its time to draw something
  glutReshapeFunc(myReshape);				// function to run when the window gets resized

  glutKeyboardFunc(spacebar);
  
  glutMainLoop();							// infinite loop that will keep drawing and resizing
  // and whatever else

  return 0;
}

