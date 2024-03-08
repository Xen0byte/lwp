#include <SDL2/SDL.h>

#include "main.h"

static int lerp(int a, int b, float t)
{
  if (t > 1) t = 1;
  return (int)((float)a + (float)t * ((float)b - (float)a));
}

void runWallpaperLoop(App *app)
{
  int quit = 0;

  SDL_Event event;

  int mx = 0;
  int my = 0;

  while (!quit)
  {
    static int currentX = 0;
    static int currentY = 0;

    static int lastTicks = 0;

    int   ticks = SDL_GetTicks();
    float dT    = (ticks - lastTicks) / 1000.0f;
    lastTicks   = ticks;

#ifdef __WIN32
    POINT mPos;
    GetCursorPos(&mPos);
    mx = mPos.x - GetSystemMetrics(SM_XVIRTUALSCREEN);
    my = mPos.y - GetSystemMetrics(SM_YVIRTUALSCREEN);
#else
    SDL_GetGlobalMouseState(&mx, &my);
#endif

    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT) quit = 1;

    currentX = lerp(currentX, mx, dT * 4);  // 4: smooth
    currentY = lerp(currentY, my, dT * 4);

    for (int m = 0; m < app->monitorsCount; m++)
    {
      if (!app->monitors[m].info.config.loaded || !app->monitors[m].wlp.info.config.loaded)
        continue;

      int relativeCurrentX = currentX - app->monitors[m].info.bounds.x;
      int relativeCurrentY = currentY - app->monitors[m].info.bounds.y;

      if (relativeCurrentX < 0) relativeCurrentX = 0;
      if (relativeCurrentY < 0) relativeCurrentY = 0;
      if (relativeCurrentX > app->monitors[m].info.bounds.w)
        relativeCurrentX = app->monitors[m].info.bounds.w;
      if (relativeCurrentY > app->monitors[m].info.bounds.h)
        relativeCurrentY = app->monitors[m].info.bounds.h;

      if (SDL_SetRenderTarget(app->renderer, app->monitors[m].wlp.tex) != 0)
      {
        lwpLog(LOG_ERROR, "Error setting the renderer target: %s", SDL_GetError());
        quit = 1;
      }
      SDL_RenderClear(app->renderer);

      for (int i = 0; i < app->monitors[m].wlp.info.config.layersCount; i++)
      {
        SDL_Rect src = {
            .x = 0,
            .y = 0,
            .w = app->monitors[m].wlp.originalW,
            .h = app->monitors[m].wlp.originalH,
        };

        int x =
            -((relativeCurrentX - app->monitors[m].info.bounds.w / 2) *
              app->monitors[m].wlp.info.config.layerConfigs[i].sensitivityX);
        int y =
            -((relativeCurrentY - app->monitors[m].info.bounds.h / 2) *
              app->monitors[m].wlp.info.config.layerConfigs[i].sensitivityY);

        for (int k = -app->monitors[m].wlp.info.config.repeatY;
             k <= app->monitors[m].wlp.info.config.repeatY;
             k++)
        {
          for (int j = -app->monitors[m].wlp.info.config.repeatX;
               j <= app->monitors[m].wlp.info.config.repeatX;
               j++)
          {
            SDL_Rect dest = {
                .x = x + j * app->monitors[m].info.config.wlpBounds.w,
                .y = y + k * app->monitors[m].info.config.wlpBounds.h,
                .w = app->monitors[m].info.config.wlpBounds.w,
                .h = app->monitors[m].info.config.wlpBounds.h,
            };

            if (SDL_RenderCopy(app->renderer, app->monitors[m].wlp.layers[i].tex, &src, &dest) != 0)
            {
              lwpLog(LOG_ERROR, "Error rendering copy: %s", SDL_GetError());
              quit = 1;
            }
          }
        }
      }

      if (SDL_SetRenderTarget(app->renderer, app->monitors[m].tex) != 0)
      {
        lwpLog(LOG_ERROR, "Error setting the renderer target: %s", SDL_GetError());
        quit = 1;
      }

      SDL_Rect src = {
          .x = 0,
          .y = 0,
          .w = app->monitors[m].info.config.wlpBounds.w,
          .h = app->monitors[m].info.config.wlpBounds.h,
      };

      SDL_Rect dest = {
          .x = app->monitors[m].info.config.wlpBounds.x,
          .y = app->monitors[m].info.config.wlpBounds.y,
          .w = app->monitors[m].info.config.wlpBounds.w,
          .h = app->monitors[m].info.config.wlpBounds.h,
      };

      if (SDL_RenderCopy(app->renderer, app->monitors[m].wlp.tex, &src, &dest) != 0)
      {
        lwpLog(LOG_ERROR, "Error rendering copy: %s", SDL_GetError());
        quit = 1;
      }

      SDL_SetRenderTarget(app->renderer, NULL);

      SDL_Rect finalSrc = {
          .x = 0,
          .y = 0,
          .w = app->monitors[m].info.bounds.w,
          .h = app->monitors[m].info.bounds.h,
      };

      SDL_Rect finalDest = {
          .x = app->monitors[m].info.bounds.x,
          .y = app->monitors[m].info.bounds.y,
          .w = app->monitors[m].info.bounds.w,
          .h = app->monitors[m].info.bounds.h,
      };

      if (SDL_RenderCopy(app->renderer, app->monitors[m].tex, &finalSrc, &finalDest) != 0)
      {
        lwpLog(LOG_ERROR, "Error rendering copy: %s", SDL_GetError());
        quit = 1;
      }
    }
    SDL_RenderPresent(app->renderer);
    SDL_Delay(1000 / app->config.targetFps);
  }
}
