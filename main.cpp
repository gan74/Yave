#include <iostream>


#include <SDL2/SDL.h>
#include <n/graphics/ImageLoader.h>
#include <n/graphics/gl/Context.h>
#include <n/graphics/gl/Texture.h>
#include <n/graphics/gl/GL.h>
#include <n/graphics/gl/Buffer.h>
#include <n/graphics/gl/TriangleBuffer.h>
#include <n/graphics/gl/VertexArrayObject.h>

using namespace n;
using namespace n::graphics;
using namespace n::math;
using namespace n::core;

SDL_Window *createWindow() {
	SDL_Window *mainWindow = 0;
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fatal("Unable to initialize SDL");
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	if(!(mainWindow = SDL_CreateWindow("n 2.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN))) {
		fatal("Unable to create window");
	}
	SDL_GL_CreateContext(mainWindow);
	SDL_GL_SetSwapInterval(0);

	gl::Context::getContext();

	return mainWindow;
}

bool run(SDL_Window *mainWindow) {
	SDL_GL_SwapWindow(mainWindow);
	SDL_Event e;
	bool cc = true;
	while(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
			cc = false;
			break;
		}
	}
	return cc;
}


int main(int, char **) {
	SDL_Window *win = createWindow();

	Image image = ImageLoader::load("mq1.png", false);
	gl::Texture tex(image);

	gl::TriangleBuffer<> tris;
	tris.append(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1));
	gl::VertexArrayObject<> vao(tris);

	while(run(win)) {

		tex.bind();
		glClear(GL_COLOR_BUFFER_BIT);
		/*glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
			glVertex2f(-1, -1);
				glTexCoord2f(0, 0);
			glVertex2f(1, -1);
				glTexCoord2f(1, 0);
			glVertex2f(1, 1);
				glTexCoord2f(1, 1);
			glVertex2f(-1, 1);
				glTexCoord2f(0, 1);
		glEnd();*/

		gl::Context::getContext()->processTasks();
		glFlush();
		if(glGetError()) {
			fatal("GL error");
		}
	}
	return 0;
}


