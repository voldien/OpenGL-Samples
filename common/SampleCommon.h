#ifndef _COMMON_H_
#define _COMMON_H_ 1
#include<SDL2/SDL.h>

extern void initWindowManager();
extern SDL_Window *createWindow(const char *ctitle);
extern void deleteWindow(SDL_Window* window);

#endif
