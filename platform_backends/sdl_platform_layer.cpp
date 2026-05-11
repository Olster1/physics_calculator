
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"


#include <stdbool.h>
#include "../platform.h"
#include "../easy_files.h"

static SDL_Window* window = NULL;
static int global_default_window_size_x = 1920;
static int global_default_window_size_y = 1080;

enum MouseButtonType {
  MOUSE_BUTTON_LEFT_CLICK,
  MOUSE_BUTTON_RIGHT_CLICK,
  MOUSE_BUTTON_MIDDLE_CLICK,

  MOUSE_BUTTON_TYPE_COUNT,
};

enum MouseKeyState {
  MOUSE_BUTTON_NONE,
  MOUSE_BUTTON_PRESSED,
  MOUSE_BUTTON_DOWN,
  MOUSE_BUTTON_RELEASED,
};

void platform_setWindowSize(int w, int h) {
  SDL_SetWindowSize(window, w, h);
}
void platform_setWindowPos(int w, int h) {
  SDL_SetWindowPosition(window, w, h);
}

char *platform_getClipBoardText() {
  char *result = SDL_GetClipboardText();

  return result;
}

void platform_freeClipBoardText(char *txt) {
  SDL_free(txt);
}

#include "../backend_renderer/opengl_backend_renderer.cpp"
// #include "../backend_renderer/sdl_backend_renderer.cpp"
#include "../main.cpp"



void updateMouseButton(u32 mouseState, GameState *gameState, MouseButtonType index, u32 sdlButtonType) {
  if(mouseState & SDL_BUTTON_MASK(sdlButtonType)) {
    if(gameState->mouseBtn[index] == MOUSE_BUTTON_NONE) {
      gameState->mouseBtn[index] = MOUSE_BUTTON_PRESSED;
    } else if(gameState->mouseBtn[index] == MOUSE_BUTTON_PRESSED) {
      gameState->mouseBtn[index] = MOUSE_BUTTON_DOWN;
    }
  } else {
    if(gameState->mouseBtn[index] == MOUSE_BUTTON_DOWN || gameState->mouseBtn[index] == MOUSE_BUTTON_PRESSED) {
      gameState->mouseBtn[index] = MOUSE_BUTTON_RELEASED;
    } else {
      gameState->mouseBtn[index] = MOUSE_BUTTON_NONE;
    }
  }
}

void getMouseData(GameState *gameState) {
  int w;
  int h;
  SDL_GetWindowSize(window, &w, &h);

  gameState->aspectRatioWindow_y_over_x = (float)h / (float)w;

  float x;
  float y;
  Uint32 mouseState = SDL_GetMouseState(&x, &y);
  gameState->mouseP_screenSpace.x = (float)x;
  gameState->mouseP_screenSpace.y = (float)(-y); //NOTE: Bottom corner is origin

  gameState->mouseP_01.x = gameState->mouseP_screenSpace.x / w;
  gameState->mouseP_01.y = (gameState->mouseP_screenSpace.y / h) + 1.0f;

  updateMouseButton(mouseState, gameState, MOUSE_BUTTON_LEFT_CLICK, SDL_BUTTON_LEFT);
  updateMouseButton(mouseState, gameState, MOUSE_BUTTON_MIDDLE_CLICK, SDL_BUTTON_MIDDLE);
  updateMouseButton(mouseState, gameState, MOUSE_BUTTON_RIGHT_CLICK, SDL_BUTTON_RIGHT);
}

static Uint32 lastTicks = 0;

struct UpdateLoopData {
  GameState *gameState;
  BackendRenderer *backendRenderer;
  int w;
  int h;
};

static void updateFrame(void* arg) {
  UpdateLoopData *data = (UpdateLoopData *)arg;
  BackendRenderer *backendRenderer = data->backendRenderer;
  GameState *gameState = data->gameState;
  Uint32 now = SDL_GetTicks();
  gameState->dt = (float)(now - lastTicks) / 1000.0f;
  lastTicks = now;

  getMouseData(gameState);

  updateGame(gameState);
  processRenderGroup(&gameState->renderer, make_float2(data->w, data->h), backendRenderer);

  backend_render_swapFrame(window);

}

int main(int argc, char** argv) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  int flags = SDL_WINDOW_RESIZABLE;

  if(OPENGL_BUILD) {
    flags |= SDL_WINDOW_OPENGL;
  }

  int w = 1920;
  int h = 1080;

  window = SDL_CreateWindow(DEFINED_FILE_NAME,
    w, h, flags);
  if (!window) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return 1;
  }

  BackendRenderer backendRenderer = {};
  backend_render_init(window, &backendRenderer);

  backend_render_getOutputSize(&w, &h);
  platform_setWindowSize(w, h);

  global_default_window_size_x = w;
  global_default_window_size_y = h;

  //NOTE: Seed random sequence
  srand((unsigned int)time(NULL));

  initMemoryArenas();
  GameState *gameState = allocateGameState(&backendRenderer);
  gameState->aspectRatioWindow_y_over_x = (float)h / (float)w;

  lastTicks = SDL_GetTicks();

  SDL_StartTextInput(window);

  bool running = true;
  while (running) {
    SDL_Event e;
    gameState->scrollWheelDelta.x = 0;
    gameState->scrollWheelDelta.y = 0;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) running = false;
      if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        gameState->scrollWheelDelta.y = e.wheel.y;
        gameState->scrollWheelDelta.x = e.wheel.x;
      }
      else if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
          if(gameState->mode == INTERACTION_MODE_DEFAULT) {
            gameState->enterPressed = true;
          } else if(gameState->mode == INTERACTION_MODE_PICK_THEME) {
            gameState->mode = INTERACTION_MODE_DEFAULT;

          }
        }
        if(e.key.key == SDLK_V && (e.key.mod & SDL_KMOD_GUI)) {
          char *text = platform_getClipBoardText();
          stringBuffer_insertString(&gameState->stringBuffer, text);
          gameState->currentCompilerError = 0;
          if(text) {
              platform_freeClipBoardText(text);
          }
        }
        if(e.key.key == SDLK_P && (e.key.mod & SDL_KMOD_GUI)) {
          gameState->mode = INTERACTION_MODE_PICK_THEME;
        }
        if(e.key.key == SDLK_ESCAPE) {
          gameState->mode = INTERACTION_MODE_DEFAULT;
          gameState->currentCompilerError = 0;
        }
        if (e.key.key == SDLK_LEFT) {
          if(gameState->mode == INTERACTION_MODE_DEFAULT) {
            if((e.key.mod & SDL_KMOD_LGUI) && gameState->stringBuffer.string) {
              gameState->stringBuffer.cursor = 0;
            } else if((e.key.mod & SDL_KMOD_LALT) && gameState->stringBuffer.string) {
              //NOTE: Move via tokens
              EasyTokenizer tokenizer = lexBeginParsing(gameState->stringBuffer.string, EASY_LEX_OPTION_EAT_WHITE_SPACE_EXCEPT_NEW_LINE);
              char *lastPos = gameState->stringBuffer.string;
              char *cursorAt = gameState->stringBuffer.string + gameState->stringBuffer.cursor;
              bool parsing = true;
              while(parsing) {
                EasyToken t = lexGetNextToken(&tokenizer);
                if(t.type == TOKEN_NULL_TERMINATOR || t.type == TOKEN_NEWLINE) {
                    gameState->stringBuffer.cursor -= (cursorAt - lastPos);
                    parsing = false;
                } else {
                  //NOTE: Get the position
                  if(tokenizer.src >= cursorAt) {
                    gameState->stringBuffer.cursor -= (cursorAt - lastPos);
                    parsing = false;
                  }
                  lastPos = tokenizer.src;
                }
              }
            } else {
              stringBuffer_cursorLeft(&gameState->stringBuffer, 1);
            }
          }
        }
        if (e.key.key == SDLK_RIGHT) {
          if(gameState->mode == INTERACTION_MODE_DEFAULT) {
            if((e.key.mod & SDL_KMOD_LGUI) && gameState->stringBuffer.string) {
              int maxLength = easyString_getStringLength_utf8(gameState->stringBuffer.string);
              gameState->stringBuffer.cursor = maxLength;
            } else if((e.key.mod & SDL_KMOD_LALT) && gameState->stringBuffer.string) {
              //NOTE: Move via tokens
              EasyTokenizer tokenizer = lexBeginParsing(gameState->stringBuffer.string, EASY_LEX_OPTION_EAT_WHITE_SPACE_EXCEPT_NEW_LINE);
              char *cursorAt = gameState->stringBuffer.string + gameState->stringBuffer.cursor;
              bool parsing = true;
              while(parsing) {
                EasyToken t = lexGetNextToken(&tokenizer);
                if(t.type == TOKEN_NULL_TERMINATOR || t.type == TOKEN_NEWLINE) {
                    parsing = false;
                } else {
                  //NOTE: Get the position
                  if(tokenizer.src > cursorAt) {
                    gameState->stringBuffer.cursor += (tokenizer.src - cursorAt);
                    parsing = false;
                  }
                }
              }

            } else {
              stringBuffer_cursorRight(&gameState->stringBuffer, 1);
            }
          }
        }
        if (e.key.key == SDLK_DOWN) {
          if(gameState->mode == INTERACTION_MODE_DEFAULT) {
            if(gameState->historyAt < gameState->bufferHistory.count) {
              gameState->historyAt++;
              if(gameState->historyAt < gameState->bufferHistory.count) {
                clearStringBuffer(&gameState->stringBuffer);
                stringBuffer_insertString(&gameState->stringBuffer, gameState->bufferHistory[gameState->historyAt].output);
                gameState->currentCompilerError = 0;
              } else {
                clearStringBuffer(&gameState->stringBuffer);
              }
            } else {
                clearStringBuffer(&gameState->stringBuffer);
            }
          } else if(gameState->mode == INTERACTION_MODE_PICK_THEME) {
            gameState->settingsToSave.themeIndex++;
            gameState->settingsToSave.themeIndex %= arrayCount(gameState->colorPallettes.pallettes);
            gameState->colorPallette = &gameState->colorPallettes.pallettes[gameState->settingsToSave.themeIndex];
            saveSettingsFile(&gameState->settingsToSave);
          }
        }
        if (e.key.key == SDLK_UP) {
          if(gameState->mode == INTERACTION_MODE_DEFAULT) {
            if(gameState->historyAt > 0) {
              gameState->historyAt--;
              clearStringBuffer(&gameState->stringBuffer);
              stringBuffer_insertString(&gameState->stringBuffer, gameState->bufferHistory[gameState->historyAt].output);
              gameState->currentCompilerError = 0;
            }
          } else if(gameState->mode == INTERACTION_MODE_PICK_THEME) {
            gameState->settingsToSave.themeIndex--;
            if(gameState->settingsToSave.themeIndex < 0) {
              gameState->settingsToSave.themeIndex = 0;
            }
            gameState->colorPallette = &gameState->colorPallettes.pallettes[gameState->settingsToSave.themeIndex];
            saveSettingsFile(&gameState->settingsToSave);
          }
        }
        if (e.key.key == SDLK_BACKSPACE) {
          if(gameState->mode == INTERACTION_MODE_DEFAULT) {
            gameState->currentCompilerError = 0;
            stringBuffer_removeCharacter(&gameState->stringBuffer);
          }
        }
      }

      if (e.type == SDL_EVENT_WINDOW_RESIZED) {
        w = e.window.data1;
        h = e.window.data2;
        gameState->aspectRatioWindow_y_over_x = (float)h / (float)w;
        gameState->settingsToSave.windowX = w;
        gameState->settingsToSave.windowY = h;

        saveSettingsFile(&gameState->settingsToSave);

        backendRenderer_setViewport(0, 0, w, h);

      }
      if (e.type == SDL_EVENT_WINDOW_MOVED) {
        int x = e.window.data1;
        int y = e.window.data2;
        gameState->settingsToSave.windowPosX = x;
        gameState->settingsToSave.windowPosY = y;

        saveSettingsFile(&gameState->settingsToSave);
      } else if (e.type == SDL_EVENT_TEXT_INPUT) {
        stringBuffer_insertString(&gameState->stringBuffer, (char *)e.text.text);
        gameState->currentCompilerError = 0;
      }

    }
    refreshPerFrameArena();

    UpdateLoopData data = {};
    data.gameState = gameState;
    data.w = w;
    data.h = h;
    data.backendRenderer = &backendRenderer;
    updateFrame(&data);
    gameState->enterPressed = false;
  }
  SDL_Quit();

  return 0;
}
