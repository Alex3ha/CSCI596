/***********************************************************************
  Program atomv.c--ball representation of atoms.
  Required files
    atomv.h:   Include file
    md.conf:   MD configuration file containing atomic coordinates
***********************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include "atomv.h"
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>    /* Header file for the OpenGL library */
#include <GL/glu.h>   /* Header file for the GLu library */
#include <GL/glut.h>    /* Header file for the GLut library */

GLuint sphereid, trajectoryid;          /* display-list id of atom sphere geom */
GLuint atomsid;           /* display-list id of all atoms */
GLdouble fovy, aspect, near_clip, far_clip;
/* parameters for gluPerspective() */
FILE* fp;                 /* pointer to open an MD-configuration file */

int tracked_atom = 0;
#define MAX_TRAJ_POINTS 1000
int num_traj_points = 0;
TrajectoryPoint trajectory[MAX_TRAJ_POINTS];

/* Function prototypes ************************************************/
void reshape(int, int);
void makeFastNiceSphere(GLuint, double);
void makeAtoms(void);
void makeCurframeGeom(void);
void drawScene(void);
void display(void);
void initView(float*, float*);
void readConf(void);
void stressToColor(float, float*, float*, float*);
void updateTrajectory();
void drawTrajectory();
void keyboard(unsigned char, int, int);
void animateAtoms();

void animateAtoms() {
    static float t = 0.0;
    t += 0.01;

    for (int i = 0; i < natoms; i++) {
        atoms[i].crd[0] += 0.01 * sin(t + i);
        atoms[i].crd[1] += 0.01 * cos(t + i);
        atoms[i].crd[2] += 0.01 * sin(t * 0.5 + i);

        atoms[i].stress = (sin(atoms[i].crd[0]) * cos(atoms[i].crd[1]) + 1.0) / 2.0;
    }
    makeAtoms();
}

void stressToColor(float stress, float* r, float* g, float* b) {
    // Map stress to a color gradient (blue->green->red)
    if (stress < 0.5) {
        *r = 0.0;
        *g = 2.0 * stress;
        *b = 1.0 - 2.0 * stress;
    }
    else {
        *r = 2.0 * (stress - 0.5);
        *g = 1.0 - 2.0 * (stress - 0.5);
        *b = 0.0;
    }
}

void updateTrajectory() {
    static float prev_x = 0, prev_y = 0, prev_z = 0;
    float curr_x = atoms[tracked_atom].crd[0];
    float curr_y = atoms[tracked_atom].crd[1];
    float curr_z = atoms[tracked_atom].crd[2];

    if (curr_x != prev_x || curr_y != prev_y || curr_z != prev_z) {
        if (num_traj_points < MAX_TRAJ_POINTS) {
            trajectory[num_traj_points].x = curr_x;
            trajectory[num_traj_points].y = curr_y;
            trajectory[num_traj_points].z = curr_z;
            num_traj_points++;
        }
        else {
            // Shift array to make room for new point
            for (int i = 0; i < MAX_TRAJ_POINTS - 1; i++) {
                trajectory[i] = trajectory[i + 1];
            }
            trajectory[MAX_TRAJ_POINTS - 1].x = curr_x;
            trajectory[MAX_TRAJ_POINTS - 1].y = curr_y;
            trajectory[MAX_TRAJ_POINTS - 1].z = curr_z;
        }

        prev_x = curr_x;
        prev_y = curr_y;
        prev_z = curr_z;
    }
}

void drawTrajectory() {
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 0.0);  // Yellow trajectory
    glLineWidth(2.0);

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < num_traj_points; i++) {
        glVertex3f(trajectory[i].x, trajectory[i].y, trajectory[i].z);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

/**********************************************************************/
void reshape(int w, int h) {
    /***********************************************************************
      Callback for glutReshapeFunc()
    ***********************************************************************/
    /* set the GL viewport to match the full size of the window */
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    aspect = w / (float)h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, aspect, near_clip, far_clip);
    glMatrixMode(GL_MODELVIEW);
}

/**********************************************************************/
void makeFastNiceSphere(GLuint listid, double radius) {
    /***********************************************************************
    Called once to generate and compile sphere geometry into the given
    display list id.
    ***********************************************************************/
    int i, j;
    float lon, lat;
    float loninc, latinc;
    float x, y, z;

    loninc = 2 * M_PI / nlon;
    latinc = M_PI / nlat;

    glNewList(listid, GL_COMPILE);

    /* South-pole triangular fan */
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(0, -radius, 0);
    lon = 0;
    lat = -M_PI / 2 + latinc;
    y = sin(lat);
    for (i = 0; i <= nlon; i++) {
        x = cos(lon) * cos(lat);
        z = -sin(lon) * cos(lat);
        glNormal3f(x, y, z);
        glVertex3f(x * radius, y * radius, z * radius);
        lon += loninc;
    }
    glEnd();

    /* Quadrilateral stripes to cover the sphere */
    for (j = 1; j < nlat - 1; j++) {
        lon = 0;
        glBegin(GL_QUAD_STRIP);
        for (i = 0; i <= nlon; i++) {
            x = cos(lon) * cos(lat);
            y = sin(lat);
            z = -sin(lon) * cos(lat);
            glNormal3f(x, y, z);
            glVertex3f(x * radius, y * radius, z * radius);
            x = cos(lon) * cos(lat + latinc);
            y = sin(lat + latinc);
            z = -sin(lon) * cos(lat + latinc);
            glNormal3f(x, y, z);
            glVertex3f(x * radius, y * radius, z * radius);
            lon += loninc;
        }
        glEnd();
        lat += latinc;
    }

    /* North-pole triangular fan */
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, radius, 0);
    y = sin(lat);
    lon = 0;
    for (i = 0; i <= nlon; i++) {
        x = cos(lon) * cos(lat);
        z = -sin(lon) * cos(lat);
        glNormal3f(x, y, z);
        glVertex3f(x * radius, y * radius, z * radius);
        lon += loninc;
    }
    glEnd();

    glEndList();
}

/**********************************************************************/
void makeAtoms() {
    /***********************************************************************
      Makes display-list of all atoms in the current frame using spheres.
    ***********************************************************************/
    int i;
    float r, g, b;

    glNewList(atomsid, GL_COMPILE);
    for (i = 0; i < natoms; i++) {
        glPushMatrix();
        glTranslatef(atoms[i].crd[0], atoms[i].crd[1], atoms[i].crd[2]);

        stressToColor(atoms[i].stress, &r, &g, &b);
        glColor3f(r, g, b);

        if (i == tracked_atom) {
            glColor3f(1.0, 1.0, 1.0);
        }

        glCallList(sphereid);
        glPopMatrix();
    }
    glEndList();
}

/**********************************************************************/
void makeCurframeGeom() {
    /***********************************************************************
      Reads the atoms information for the current time frame and makes the
      display-list of all the atoms' geometry.
    ***********************************************************************/
    makeAtoms();
}

/**********************************************************************/
void drawScene() {
    /***********************************************************************
      Called by display() to draw the view of the current scene.
    ***********************************************************************/
    /* Define viewing transformation */
    gluLookAt(
        (GLdouble)eye[0], (GLdouble)eye[1], (GLdouble)eye[2],
        (GLdouble)center[0], (GLdouble)center[1], (GLdouble)center[2],
        (GLdouble)up[0], (GLdouble)up[1], (GLdouble)up[2]);
    glCallList(atomsid);
    drawTrajectory();
}

/**********************************************************************/
void display() {
    /***********************************************************************
      Callback for glutDisplayFunc().  It clears the frame and depth
      buffers and draws the atoms in the current frame.
    ***********************************************************************/
    animateAtoms();
    if (tracking_enabled) {
        updateTrajectory();
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    drawScene();
    glutSwapBuffers();

    glutPostRedisplay();
}

/**********************************************************************/
void initView(float* min_ext, float* max_ext) {
    /***********************************************************************
      Initializes global viewing, lighting, and projection values.
    ***********************************************************************/
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position1[] = { 0.5, 0.5, 1.0, 0.0 };
    float dif_ext[3], dis;
    int i;

    /* Define normal light */
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position1);

    /* Enable a single OpenGL light */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* Use depth buffering for hidden surface elimination */
    glEnable(GL_DEPTH_TEST);

    /* get diagonal and average distance of extent */
    for (i = 0; i < 3; i++) dif_ext[i] = max_ext[i] - min_ext[i];
    dis = 0.0;
    for (i = 0; i < 3; i++) dis += dif_ext[i] * dif_ext[i];
    dis = (float)sqrt((double)dis);

    /* set center in world space */
    for (i = 0; i < 3; i++) center[i] = min_ext[i] + dif_ext[i] / 2.0;

    /* set initial eye & look at location in world space */
    eye[0] = center[0];
    eye[1] = center[1];
    eye[2] = center[2] + dis;
    up[0] = 0.0;
    up[1] = 1.0;
    up[2] = 0.0;

    /* set parameters for gluPerspective() */
    /* Near- & far clip-plane distances */
    near_clip = (GLdouble)(0.5 * (dis - 0.5 * dif_ext[2]));
    far_clip = (GLdouble)(2.0 * (dis + 0.5 * dif_ext[2]));
    /* Field of view */
    fovy = (GLdouble)(0.5 * dif_ext[1] / (dis - 0.5 * dif_ext[2]));
    fovy = (GLdouble)(2 * atan((double)fovy) / M_PI * 180.0);
    fovy = (GLdouble)(1.2 * fovy);

    /* Enable the color material mode */
    glEnable(GL_COLOR_MATERIAL);
}

/**********************************************************************/
void readConf() {
    /***********************************************************************
    Read atomic coordinates from an MD-configuration file & allocates
    necessary arrays.
    ***********************************************************************/
    int l, j;

    /* Open an MD-configuration file */
    fp = fopen("md.conf", "r");
    /* Read the # of atoms */
    fscanf_s(fp, "%d", &natoms);
    /* allocate atoms array */
    atoms = (AtomType*)malloc(sizeof(AtomType) * natoms);
    /* Maximum & minimum extent of system in angstroms */
    for (l = 0; l < 3; l++) fscanf_s(fp, "%f%f", &min_ext[l], &max_ext[l]);
    /* Atomic coordinates */
    for (j = 0; j < natoms; j++) {
        fscanf_s(fp, "%f %f %f", &(atoms[j].crd[0]), &(atoms[j].crd[1]),
            &(atoms[j].crd[2]));
        atoms[j].stress = (float)(
            sin(atoms[j].crd[0]) * cos(atoms[j].crd[1]) + 1.0) / 2.0;
    }
    fclose(fp);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 't':  // show tracked atom
        tracked_atom = (tracked_atom + 1) % natoms;
        tracking_enabled = true;
        printf("Tracking atom %d\n",
            tracked_atom);
        makeAtoms();
        break;
    case 'c':  // clear trajectory
        num_traj_points = 0;
        printf("Trajectory cleared\n");
        break;
    case 's': // close tracking
        tracking_enabled = false;
        printf("Tracking disabled\n");
        break;
    case 'q':  // quit
        printf("Quitting...\n");
        exit(0);
        break;
    }
    glutPostRedisplay();
}

/**********************************************************************/
int main(int argc, char** argv) {
    /**********************************************************************/

    glutInit(&argc, argv);

    /* Read atomic coordinates from an MD-configuration file */
    readConf();

    /* Set up an window */
    /* Initialize display mode */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    /* Specify window size */
    glutInitWindowSize(winx, winy);
    /* Open window */
    glutCreateWindow("Enhanced Lennard-Jones Atoms Visualization");

    /* Initialize view */
    initView(min_ext, max_ext);

    /* Set a glut callback functions */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    /* generate an OpenGL display list for single sphere */
    sphereid = glGenLists(1);
    makeFastNiceSphere(sphereid, atom_radius);

    /* generate an OpenGL display list for the atoms' geometry */
    atomsid = glGenLists(1);
    /* make the geometry of the current frame's atoms */
    makeCurframeGeom();

    /* Start main display loop */
    glutMainLoop();

    return 0;
}
/**********************************************************************/


