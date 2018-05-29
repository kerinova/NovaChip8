#include <stdio.h>
#include "glut.h"
#include "emulator.h"

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

int sizeModifier = 10;

int displayWidth = SCREEN_WIDTH * sizeModifier;
int displayHeight = SCREEN_HEIGHT * sizeModifier;
unsigned __int8 screenData[SCREEN_HEIGHT][SCREEN_WIDTH][3];

Chip8 chip8;

void Display();
void ReshapeWindow(GLsizei w, GLsizei h);
void KeyboardUp(unsigned char key, int x, int y);
void KeyboardDown(unsigned char key, int x, int y);
void SetupTexture();

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Usage: NovaChip8.exe CHIP-8application\n\n");
		return 1;
	}

	chip8.Initialize();

	if (!chip8.LoadProgram(&argv[1]))
	{
		return 1;
	}

	// Setup OpenGL
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(displayWidth, displayHeight);
	glutInitWindowPosition(320, 320);
	glutCreateWindow("CHIP-8 Emulator");

	glutDisplayFunc(Display);
	glutIdleFunc(Display);
	glutReshapeFunc(ReshapeWindow);
	glutKeyboardFunc(KeyboardDown);
	glutKeyboardUpFunc(KeyboardUp);
	SetupTexture();
	glutMainLoop();
	return 0;
}

// Setup Texture
void SetupTexture()
{
	// Clear screen
	for (int y = 0; y < SCREEN_HEIGHT; ++y)
		for (int x = 0; x < SCREEN_WIDTH; ++x)
			screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;

	// Create a texture 
	glTexImage2D(GL_TEXTURE_2D, 0, 3, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

	// Set up the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Enable textures
	glEnable(GL_TEXTURE_2D);
}

void UpdateTexture(const Chip8& chip8)
{
	// Update pixels
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 64; ++x)
			if (chip8.display[(y * 64) + x] == 0)
				screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;	// Disabled
			else
				screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 255;  // Enabled

																						// Update Texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0);		glVertex2d(0.0, 0.0);
	glTexCoord2d(1.0, 0.0); 	glVertex2d(displayWidth, 0.0);
	glTexCoord2d(1.0, 1.0); 	glVertex2d(displayWidth, displayHeight);
	glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0, displayHeight);
	glEnd();
}

void Display()
{
	chip8.Cycle();

	if (chip8.drawFlag)
	{
		// Clear framebuffer
		glClear(GL_COLOR_BUFFER_BIT);

		UpdateTexture(chip8);

		// Swap buffers!
		glutSwapBuffers();

		// Processed frame
		chip8.drawFlag = false;
	}
}

void ReshapeWindow(GLsizei w, GLsizei h)
{
	glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);

	// Resize quad
	displayWidth = w;
	displayHeight = h;
}

void KeyboardDown(unsigned char key, int x, int y)
{
	if (key == 27)    // esc
		exit(0);

	if (key == '1')		chip8.key[0x1] = 1;
	else if (key == '2')	chip8.key[0x2] = 1;
	else if (key == '3')	chip8.key[0x3] = 1;
	else if (key == '4')	chip8.key[0xC] = 1;

	else if (key == 'q')	chip8.key[0x4] = 1;
	else if (key == 'w')	chip8.key[0x5] = 1;
	else if (key == 'e')	chip8.key[0x6] = 1;
	else if (key == 'r')	chip8.key[0xD] = 1;

	else if (key == 'a')	chip8.key[0x7] = 1;
	else if (key == 's')	chip8.key[0x8] = 1;
	else if (key == 'd')	chip8.key[0x9] = 1;
	else if (key == 'f')	chip8.key[0xE] = 1;

	else if (key == 'z')	chip8.key[0xA] = 1;
	else if (key == 'x')	chip8.key[0x0] = 1;
	else if (key == 'c')	chip8.key[0xB] = 1;
	else if (key == 'v')	chip8.key[0xF] = 1;

	//printf("Press key %c\n", key);
}

void KeyboardUp(unsigned char key, int x, int y)
{
	if (key == '1')		chip8.key[0x1] = 0;
	else if (key == '2')	chip8.key[0x2] = 0;
	else if (key == '3')	chip8.key[0x3] = 0;
	else if (key == '4')	chip8.key[0xC] = 0;

	else if (key == 'q')	chip8.key[0x4] = 0;
	else if (key == 'w')	chip8.key[0x5] = 0;
	else if (key == 'e')	chip8.key[0x6] = 0;
	else if (key == 'r')	chip8.key[0xD] = 0;

	else if (key == 'a')	chip8.key[0x7] = 0;
	else if (key == 's')	chip8.key[0x8] = 0;
	else if (key == 'd')	chip8.key[0x9] = 0;
	else if (key == 'f')	chip8.key[0xE] = 0;

	else if (key == 'z')	chip8.key[0xA] = 0;
	else if (key == 'x')	chip8.key[0x0] = 0;
	else if (key == 'c')	chip8.key[0xB] = 0;
	else if (key == 'v')	chip8.key[0xF] = 0;
}
