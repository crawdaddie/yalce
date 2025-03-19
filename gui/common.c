#include "./common.h"

TTF_Font *DEFAULT_FONT;

void _render_text(const char *text, int x, int y, SDL_Color color,
                  SDL_Renderer *renderer, TTF_Font *font) {

  SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect rect = {x, y, surface->w, surface->h};
  SDL_RenderCopy(renderer, texture, NULL, &rect);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}
