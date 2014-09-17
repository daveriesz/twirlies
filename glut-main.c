
#include <stdio.h>
#include <libgen.h>
#include <GL/glut.h>

#include "twirlies.h"

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB);
	glutInitWindowSize(500,500);
	glutInitWindowPosition(100,100);
	glutCreateWindow(basename(argv[0]));

	tw_init();
	glutDisplayFunc(tw_dummy_display);
	glutIdleFunc(tw_display);
	glutReshapeFunc(tw_reshape);
	glutKeyboardFunc(tw_keyboard);
	glutVisibilityFunc(tw_visibility);
	
	glutMainLoop();
	return 0;
	
}
