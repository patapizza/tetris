/*
 * Tetris game
 * Copyright (C) 2010 Julien Odent <julien at odent dot net>
 *
 * This game is an unofficial clone of the original
 * Tetris game and is not endorsed by the
 * registered trademark owners The Tetris Company, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>

#define BLOCK_SIZE 20
#define FALL_STEP 1
#define LETTER_A 0
#define LETTER_B 1
#define LETTER_C 2
#define LETTER_D 3
#define LETTER_E 4
#define LETTER_F 5
#define LETTER_G 6
#define LETTER_H 7
#define LETTER_I 8
#define LETTER_J 9
#define LETTER_K 10
#define LETTER_L 11
#define LETTER_M 12
#define LETTER_N 13
#define LETTER_O 14
#define LETTER_P 15
#define LETTER_Q 16
#define LETTER_R 17
#define LETTER_S 18
#define LETTER_T 19
#define LETTER_U 20
#define LETTER_V 21
#define LETTER_W 22
#define LETTER_X 23
#define LETTER_Y 24
#define LETTER_Z 25
#define WALL_WIDTH 400
#define WALL_HEIGHT 600
#define PTR_NULL ((void *) 0)
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
#define SPEED 7

typedef struct Game {
  int delay;
  int level;
  int lines;
  int over;
  int paused;
  int running;
  SDL_Surface *digits[10];
  SDL_Surface *letters[26];
  SDL_Surface *screen;
  struct Shape *falling, *next;
  struct SDL_Surface *grid[SCREEN_WIDTH][SCREEN_HEIGHT];
} Game;

typedef struct Shape {
  // each shape consists in four squares, located by the upper left corner
  SDL_Rect pos[4];
  SDL_Surface *img;
  /*
   * type 0 : g.jpg
   * type 1 : i.jpg
   * type 2 : l.jpg
   * type 3 : o.jpg
   * type 4 : s.jpg
   * type 5 : t.jpg
   * type 6 : z.jpg
   */
  int type;
  int angle;
} Shape;

void check_lines(int y);
void check_lost();
int clean_up(int err);
void draw_blocks();
void draw_digit(int d, int x, int y);
void draw_number(int n, int x, int y);
void draw_right();
SDL_Surface *get_image(char *str);
void empty_line(int y);
void erase_screen();
void falling_next();
int flip_checking(SDL_Rect pos[]);
void game_new();
void game_over();
void game_pause();
int min(int y1, int y2, int y3, int y4);
void move_left();
void move_right();
void shape_draw();
void shape_fall();
void shape_flip(int clockwise);
void shape_new();

Game *game;

int main(int argc, char **argv) {
  SDL_Event event;
  Uint8 *keystate;
  game = malloc(sizeof(struct Game));
  game_new();
  if (SDL_Init(SDL_INIT_VIDEO != 0)) {
    fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
    return 1;
  }
  if ((game->screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 8, SDL_SWSURFACE)) == NULL) {
    fprintf(stderr, "Could not set SDL video mode: %s\n", SDL_GetError());
    return clean_up(1);
  }
  SDL_WM_SetCaption("Tetris", "Tetris");
  SDL_ShowCursor(SDL_DISABLE);
  while (1) {
    erase_screen();
    if (game->paused == 1)
      game_pause();
    else if (game->over == 1)
      game_over();
    else {
      shape_fall();
      shape_draw();
      draw_blocks();
    }
    draw_right();
    SDL_UpdateRect(game->screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    check_lost();
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
        game->running = 0;
      else if (game->over == 0 && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p)
        game->paused ^= 1;
    if (game->running == 0)
      break;
    keystate = SDL_GetKeyState(NULL);
    if (game->paused == 0 && game->over == 0) {
      if (keystate[SDLK_RIGHT]) {
        move_right();
        keystate[SDLK_RIGHT] = 0;
      }
      else if (keystate[SDLK_LEFT]) {
        move_left();
        keystate[SDLK_LEFT] = 0;
      }
      else if (keystate[SDLK_UP]) {
        shape_flip(1); // clockwise
        keystate[SDLK_UP] = 0;
      }
      else if (keystate[SDLK_DOWN]) {
        shape_flip(0); // counter clockwise
        keystate[SDLK_DOWN] = 0;
      }
    }
    if (game->delay - game->level > -1)
      SDL_Delay(game->delay - game->level);
  }
  free(game->falling);
  free(game->next);
  free(game);
  return clean_up(0);
}

void check_lines(int y) {
  int x, full;
  if (y == WALL_HEIGHT)
    return;
  full = 1;
  x = 0;
  while (x < WALL_WIDTH) {
    if (game->grid[x][y] == PTR_NULL) {
      full = 0;
      break;
    }
    x += BLOCK_SIZE;
  }
  if (full == 1)
    empty_line(y);
  check_lines(y + BLOCK_SIZE);
}

void check_lost() {
  int x = 0;
  while (x < WALL_WIDTH) {
    if (game->grid[x][0] != PTR_NULL)
      game->over = 1;
    x += BLOCK_SIZE;
  }
}

int clean_up(int err) {
  SDL_Quit();
  return err;
}

void draw_blocks() {
  int x, y;
  SDL_Rect pos = { 0, 0, 0, 0 };
  x = 0;
  while (x < WALL_WIDTH) {
    y = 0;
    while (y < WALL_HEIGHT) {
      if (game->grid[x][y] != PTR_NULL) {
        pos.x = x;
	pos.y = y;
        SDL_BlitSurface(game->grid[x][y], NULL, game->screen, &pos);
      }
      y += FALL_STEP;
    }
    x += BLOCK_SIZE;
  }
}

void draw_digit(int d, int x, int y) {
  SDL_Rect dest = { x, y, 0, 0 };
  SDL_BlitSurface(game->digits[d], NULL, game->screen, &dest);
}

void draw_number(int n, int x, int y) {
  while (n != 0) {
    draw_digit(n % 10, x, y);
    x -= 20;
    n /= 10;
  }
}

void draw_right() {
  SDL_Rect pos = { WALL_WIDTH + 50, 100, 0, 0 };
  SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  if (game->next->type == 0) {
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x -= BLOCK_SIZE;
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  else if (game->next->type == 1) {
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  else if (game->next->type == 2) {
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  else if (game->next->type == 3) {
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x -= BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  else if (game->next->type == 4) {
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x -= BLOCK_SIZE;
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x -= BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  else if (game->next->type == 5) {
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x -= BLOCK_SIZE;
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  else {
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.y += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
    pos.x += BLOCK_SIZE;
    SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  }
  lineRGBA(game->screen, WALL_WIDTH, 0, WALL_WIDTH, SCREEN_HEIGHT, 0, 178, 0, 255);
  SDL_BlitSurface(game->next->img, NULL, game->screen, &pos);
  pos.x = WALL_WIDTH + 20;
  pos.y = 250;
  SDL_BlitSurface(game->letters[LETTER_L], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_I], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_N], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_E], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_S], NULL, game->screen, &pos);
  (game->lines == 0) ? draw_digit(0, 530, 250) : draw_number(game->lines, 530, 250);
  pos.x = WALL_WIDTH + 20;
  pos.y = 300;
  SDL_BlitSurface(game->letters[LETTER_L], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_E], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_V], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_E], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_L], NULL, game->screen, &pos);
  draw_number(game->level, 530, 300);
}

SDL_Surface *get_image(char *str) {
  int size = strlen("images/") + strlen(str) + 1;
  char *path = malloc(size * sizeof(char));
  snprintf(path, size, "images/%s", str);
  SDL_Surface *image = IMG_Load(path);
  if (!image) {
    printf("IMG_Load: %s\n", IMG_GetError());
    clean_up(1);
  }
  free(path);
  return image;
}

void empty_line(int y) {
  if (++game->lines % 10 == 0)
    ++game->level;
  int x, yy, z;
  x = 0;
  while (x < WALL_WIDTH) {
    game->grid[x][y] = NULL;
    x += BLOCK_SIZE;
  }
  yy = y - BLOCK_SIZE;
  while (yy > 0) {
    x = 0;
    while (x < WALL_WIDTH) {
      if (game->grid[x][yy] != PTR_NULL) {
        z = yy + BLOCK_SIZE;
        while (game->grid[x][z] == PTR_NULL && z < WALL_HEIGHT) {
	  game->grid[x][z] = game->grid[x][z - BLOCK_SIZE];
	  game->grid[x][z - BLOCK_SIZE] = NULL;
	  z += BLOCK_SIZE;
	}
      }
      x += BLOCK_SIZE;
    }
    yy -= BLOCK_SIZE;
  }
}

void erase_screen() {
  SDL_Rect rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
  SDL_FillRect(game->screen, &rect, SDL_MapRGB(game->screen->format, 0x00, 0x00, 0x00));
}
void falling_next() {
  game->falling->pos[0] = game->next->pos[0];
  game->falling->pos[1] = game->next->pos[1];
  game->falling->pos[2] = game->next->pos[2];
  game->falling->pos[3] = game->next->pos[3];
  game->falling->img = game->next->img;
  game->falling->angle = game->next->angle;
  game->falling->type = game->next->type;
  shape_new();
}
int flip_checking(SDL_Rect pos[]) {
  int max_width, max_height, y, ok;
  max_width = WALL_WIDTH - BLOCK_SIZE;
  max_height = WALL_HEIGHT - BLOCK_SIZE;
  y = 0;
  ok = 1;
  while (y < BLOCK_SIZE) {
    if (game->grid[pos[0].x][pos[0].y + y] != PTR_NULL ||
      game->grid[pos[1].x][pos[1].y + y] != PTR_NULL ||
      game->grid[pos[2].x][pos[2].y + y] != PTR_NULL ||
      game->grid[pos[3].x][pos[3].y + y] != PTR_NULL)
      ok = 0;
    y += FALL_STEP;
  }
  if (pos[0].x >= 0 && pos[0].x <= max_width && pos[0].y >= 0 && pos[0].y <= max_height &&
    pos[1].x >= 0 && pos[1].x <= max_width && pos[1].y >= 0 && pos[1].y <= max_height &&
    pos[2].x >= 0 && pos[2].x <= max_width && pos[2].y >= 0 && pos[2].y <= max_height &&
    pos[3].x >= 0 && pos[3].x <= max_width && pos[3].y >= 0 && pos[3].y <= max_height &&
    ok == 1)
    return 1;
  return 0;
}

void game_new() {
  int x, y;
  char *str = malloc(6 * sizeof(char));
  // initializing the full grid as empty squares
  x = 0;
  while (x < WALL_WIDTH) {
    y = 0;
    while (y < WALL_HEIGHT) {
      game->grid[x][y] = NULL;
      y += FALL_STEP;
    }
    x += BLOCK_SIZE;
  }
  x = 0;
  while (x < 10) {
    snprintf(str, 6, "%d.jpg", x);
    game->digits[x++] = get_image(str);
  }
  free(str);
  game->letters[LETTER_A] = get_image("A.jpg");
  game->letters[LETTER_B] = get_image("B.jpg");
  game->letters[LETTER_C] = get_image("C.jpg");
  game->letters[LETTER_D] = get_image("D.jpg");
  game->letters[LETTER_E] = get_image("E.jpg");
  game->letters[LETTER_F] = get_image("F.jpg");
  game->letters[LETTER_G] = get_image("G.jpg");
  game->letters[LETTER_H] = get_image("H.jpg");
  game->letters[LETTER_I] = get_image("I.jpg");
  game->letters[LETTER_J] = get_image("J.jpg");
  game->letters[LETTER_K] = get_image("K.jpg");
  game->letters[LETTER_L] = get_image("L.jpg");
  game->letters[LETTER_M] = get_image("M.jpg");
  game->letters[LETTER_N] = get_image("N.jpg");
  game->letters[LETTER_O] = get_image("O.jpg");
  game->letters[LETTER_P] = get_image("P.jpg");
  game->letters[LETTER_Q] = get_image("Q.jpg");
  game->letters[LETTER_R] = get_image("R.jpg");
  game->letters[LETTER_S] = get_image("S.jpg");
  game->letters[LETTER_T] = get_image("T.jpg");
  game->letters[LETTER_U] = get_image("U.jpg");
  game->letters[LETTER_V] = get_image("V.jpg");
  game->letters[LETTER_W] = get_image("W.jpg");
  game->letters[LETTER_X] = get_image("X.jpg");
  game->letters[LETTER_Y] = get_image("Y.jpg");
  game->letters[LETTER_Z] = get_image("Z.jpg");
  game->level = 1;
  game->lines = 0;
  game->over = 0;
  game->paused = 0;
  game->running = 1;
  game->delay = SPEED;
  game->next = malloc(sizeof(struct Shape));
  shape_new();
  game->falling = malloc(sizeof(struct Shape));
  falling_next();
}

void game_over() {
  SDL_Rect pos = { (int) WALL_WIDTH / 2 - 75, (int) WALL_HEIGHT / 2, 0, 0 };
  SDL_BlitSurface(game->letters[LETTER_G], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_A], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_M], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_E], NULL, game->screen, &pos);
  pos.x += 30;
  SDL_BlitSurface(game->letters[LETTER_O], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_V], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_E], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_R], NULL, game->screen, &pos);
}

void game_pause() {
  SDL_Rect pos = { (int) WALL_WIDTH / 2 - 45, (int) WALL_HEIGHT / 2, 0, 0 };
  SDL_BlitSurface(game->letters[LETTER_P], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_A], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_U], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_S], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_E], NULL, game->screen, &pos);
  pos.x += 15;
  SDL_BlitSurface(game->letters[LETTER_D], NULL, game->screen, &pos);
}

int min(int y1, int y2, int y3, int y4) {
   int min1, min2;
   min1 = (y1 < y2) ? y1 : y2;
   min2 = (y3 < y4) ? y3 : y4;
   return (min1 < min2) ? min1 : min2;
}
void move_left() {
  int ok, y;
  y = 0;
  ok = 1;
  while (y < BLOCK_SIZE) {
    if (game->grid[game->falling->pos[0].x - BLOCK_SIZE][game->falling->pos[0].y + y] != PTR_NULL ||
      game->grid[game->falling->pos[1].x - BLOCK_SIZE][game->falling->pos[1].y + y] != PTR_NULL ||
      game->grid[game->falling->pos[2].x - BLOCK_SIZE][game->falling->pos[2].y + y] != PTR_NULL ||
      game->grid[game->falling->pos[3].x - BLOCK_SIZE][game->falling->pos[3].y + y] != PTR_NULL)
      ok = 0;
    y += FALL_STEP;
  }
  if (game->falling->pos[0].x >= BLOCK_SIZE && game->falling->pos[1].x >= BLOCK_SIZE && game->falling->pos[2].x >= BLOCK_SIZE && game->falling->pos[3].x >= BLOCK_SIZE && ok == 1) {
    game->falling->pos[0].x -= BLOCK_SIZE;
    game->falling->pos[1].x -= BLOCK_SIZE;
    game->falling->pos[2].x -= BLOCK_SIZE;
    game->falling->pos[3].x -= BLOCK_SIZE;
  }
}

void move_right() {
  int ok, y, max;
  max = WALL_WIDTH - BLOCK_SIZE;
  y = 0;
  ok = 1;
  while (y < BLOCK_SIZE) {
    if (game->grid[game->falling->pos[0].x + BLOCK_SIZE][game->falling->pos[0].y + y] != PTR_NULL ||
      game->grid[game->falling->pos[1].x + BLOCK_SIZE][game->falling->pos[1].y + y] != PTR_NULL ||
      game->grid[game->falling->pos[2].x + BLOCK_SIZE][game->falling->pos[2].y + y] != PTR_NULL ||
      game->grid[game->falling->pos[3].x + BLOCK_SIZE][game->falling->pos[3].y + y] != PTR_NULL)
      ok = 0;
    y += FALL_STEP;
  }
  if (game->falling->pos[0].x < max && game->falling->pos[1].x < max && game->falling->pos[2].x < max && game->falling->pos[3].x < max && ok == 1) {
    game->falling->pos[0].x += BLOCK_SIZE;
    game->falling->pos[1].x += BLOCK_SIZE;
    game->falling->pos[2].x += BLOCK_SIZE;
    game->falling->pos[3].x += BLOCK_SIZE;
  }
}

void shape_draw() {
  SDL_BlitSurface(game->falling->img, NULL, game->screen, &game->falling->pos[0]);
  SDL_BlitSurface(game->falling->img, NULL, game->screen, &game->falling->pos[1]);
  SDL_BlitSurface(game->falling->img, NULL, game->screen, &game->falling->pos[2]);
  SDL_BlitSurface(game->falling->img, NULL, game->screen, &game->falling->pos[3]);
}

void shape_fall() {
  int max = WALL_HEIGHT - BLOCK_SIZE;
  if (game->falling->pos[0].y < max && game->falling->pos[1].y < max && game->falling->pos[2].y < max && game->falling->pos[3].y < max &&
    game->grid[game->falling->pos[0].x][game->falling->pos[0].y + BLOCK_SIZE] == PTR_NULL &&
    game->grid[game->falling->pos[1].x][game->falling->pos[1].y + BLOCK_SIZE] == PTR_NULL &&
    game->grid[game->falling->pos[2].x][game->falling->pos[2].y + BLOCK_SIZE] == PTR_NULL &&
    game->grid[game->falling->pos[3].x][game->falling->pos[3].y + BLOCK_SIZE] == PTR_NULL) {
    game->falling->pos[0].y += FALL_STEP;
    game->falling->pos[1].y += FALL_STEP;
    game->falling->pos[2].y += FALL_STEP;
    game->falling->pos[3].y += FALL_STEP;
  }
  else { // stuck, let's fall a new shape
    game->grid[game->falling->pos[0].x][game->falling->pos[0].y] = game->falling->img;
    game->grid[game->falling->pos[1].x][game->falling->pos[1].y] = game->falling->img;
    game->grid[game->falling->pos[2].x][game->falling->pos[2].y] = game->falling->img;
    game->grid[game->falling->pos[3].x][game->falling->pos[3].y] = game->falling->img;
    check_lines(min(game->falling->pos[0].y, game->falling->pos[1].y, game->falling->pos[2].y, game->falling->pos[3].y));
    falling_next();
  }
}
void shape_new() {
  SDL_Rect pos = { (int) (WALL_WIDTH / 2), 0, 0, 0 };
  game->next->angle = 0;
  srand(time(NULL));
  game->next->type = (int) (7.0 * rand() / (RAND_MAX + 1.0));
  game->next->pos[0] = pos;
  if (game->next->type == 0) { // g.jpg
    game->next->img = get_image("g.jpg");
    pos.x += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.x -= BLOCK_SIZE;
    pos.y += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.y += BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
  else if (game->next->type == 1) { // i.jpg
    game->next->img = get_image("i.jpg");
    pos.y += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.y += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.y += BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
  else if (game->next->type == 2) { // l.jpg
    game->next->img = get_image("l.jpg");
    pos.y += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.y += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.x += BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
  else if (game->next->type == 3) { // o.jpg
    game->next->img = get_image("o.jpg");
    pos.x += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.y += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.x -= BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
  else if (game->next->type == 4) { // s.jpg
    game->next->img = get_image("s.jpg");
    pos.x += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.x -= BLOCK_SIZE;
    pos.y += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.x -= BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
  else if (game->next->type == 5) { // t.jpg
    game->next->img = get_image("t.jpg");
    pos.x += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.x += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.x -= BLOCK_SIZE;
    pos.y += BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
  else { // z.jpg
    game->next->img = get_image("z.jpg");
    pos.x += BLOCK_SIZE;
    game->next->pos[1] = pos;
    pos.y += BLOCK_SIZE;
    game->next->pos[2] = pos;
    pos.x += BLOCK_SIZE;
    game->next->pos[3] = pos;
  }
}

void shape_flip(int clockwise) {
  SDL_Rect pos[4];
  pos[0] = game->falling->pos[0];
  pos[1] = game->falling->pos[1];
  pos[2] = game->falling->pos[2];
  pos[3] = game->falling->pos[3];
  if (game->falling->type == 0) { // g.jpg
    if (game->falling->angle == 0)
      if (clockwise == 0) {
        pos[0].x += BLOCK_SIZE;
	pos[0].y += 2 * BLOCK_SIZE;
	pos[1].x += BLOCK_SIZE;
	pos[1].y += 2 * BLOCK_SIZE;
      }
      else {
        pos[2].x += 2 * BLOCK_SIZE;
	pos[2].y -= BLOCK_SIZE;
	pos[3].x += 2 * BLOCK_SIZE;
	pos[3].y -= BLOCK_SIZE;
      }
    else if (game->falling->angle == 1)
      if (clockwise == 0) {
        pos[2].x -= 2 * BLOCK_SIZE;
	pos[2].y += BLOCK_SIZE;
	pos[3].x -= 2 * BLOCK_SIZE;
	pos[3].y += BLOCK_SIZE;
      }
      else {
        pos[0].x += BLOCK_SIZE;
	pos[0].y += 2 * BLOCK_SIZE;
	pos[1].x += BLOCK_SIZE;
	pos[1].y += 2 * BLOCK_SIZE;
      }
    else if (game->falling->angle == 2)
      if (clockwise == 0) {
        pos[0].x -= BLOCK_SIZE;
	pos[0].y -= 2 * BLOCK_SIZE;
	pos[1].x -= BLOCK_SIZE;
	pos[1].y -= 2 * BLOCK_SIZE;
      }
      else {
        pos[2].x -= 2 * BLOCK_SIZE;
	pos[2].y += BLOCK_SIZE;
	pos[3].x -= 2 * BLOCK_SIZE;
	pos[3].y += BLOCK_SIZE;
      }
    else
      if (clockwise == 0) {
        pos[2].x += 2 * BLOCK_SIZE;
	pos[2].y -= BLOCK_SIZE;
	pos[3].x += 2 * BLOCK_SIZE;
	pos[3].y -= BLOCK_SIZE;
      }
      else {
        pos[0].x -= BLOCK_SIZE;
	pos[0].y -= 2 * BLOCK_SIZE;
	pos[1].x -= BLOCK_SIZE;
	pos[1].y -= 2 * BLOCK_SIZE;
      }
  }
  else if (game->falling->type == 1) // i.jpg
    if (game->falling->angle == 0) {
      pos[0].x -= BLOCK_SIZE;
      pos[0].y += BLOCK_SIZE;
      pos[2].x += BLOCK_SIZE;
      pos[2].y -= BLOCK_SIZE;
      pos[3].x += 2 * BLOCK_SIZE;
      pos[3].y -= 2 * BLOCK_SIZE;
    }
    else {
      pos[0].x += BLOCK_SIZE;
      pos[0].y -= BLOCK_SIZE;
      pos[2].x -= BLOCK_SIZE;
      pos[2].y += BLOCK_SIZE;
      pos[3].x -= 2 * BLOCK_SIZE;
      pos[3].y += 2 * BLOCK_SIZE;
    }
  else if (game->falling->type == 2) { // l.jpg
    if (game->falling->angle == 0)
      if (clockwise == 0) {
        pos[0].x += 2 * BLOCK_SIZE;
	pos[0].y += BLOCK_SIZE;
	pos[1].x += 2 * BLOCK_SIZE;
	pos[1].y += BLOCK_SIZE;
      }
      else {
        pos[2].x += BLOCK_SIZE;
	pos[2].y -= 2 * BLOCK_SIZE;
	pos[3].x += BLOCK_SIZE;
	pos[3].y -= 2 * BLOCK_SIZE;
      }
    else if (game->falling->angle == 1)
      if (clockwise == 0) {
        pos[2].x -= BLOCK_SIZE;
	pos[2].y += 2 * BLOCK_SIZE;
	pos[3].x -= BLOCK_SIZE;
	pos[3].y += 2 * BLOCK_SIZE;
      }
      else {
        pos[0].x += 2 * BLOCK_SIZE;
	pos[0].y += BLOCK_SIZE;
	pos[1].x += 2 * BLOCK_SIZE;
	pos[1].y += BLOCK_SIZE;
      }
    else if (game->falling->angle == 2)
      if (clockwise == 0) {
        pos[0].x -= 2 * BLOCK_SIZE;
	pos[0].y -= BLOCK_SIZE;
	pos[1].x -= 2 * BLOCK_SIZE;
	pos[1].y -= BLOCK_SIZE;
      }
      else {
        pos[2].x -= BLOCK_SIZE;
	pos[2].y += 2 * BLOCK_SIZE;
        pos[3].x -= BLOCK_SIZE;
	pos[3].y += 2 * BLOCK_SIZE;
      }
    else
      if (clockwise == 0) {
        pos[2].x += BLOCK_SIZE;
	pos[2].y -= 2 * BLOCK_SIZE;
	pos[3].x += BLOCK_SIZE;
	pos[3].y -= 2 * BLOCK_SIZE;
      }
      else {
        pos[0].x -= 2 * BLOCK_SIZE;
	pos[0].y -= BLOCK_SIZE;
	pos[1].x -= 2 * BLOCK_SIZE;
	pos[1].y -= BLOCK_SIZE;
      }
  }
  else if (game->falling->type == 3) // o.jpg -- nothing to do
    ;
  else if (game->falling->type == 4) // s.jpg
    if (game->falling->angle == 0) {
      pos[2].y -= 2 * BLOCK_SIZE;
      pos[3].x += 2 * BLOCK_SIZE;
    }
    else {
      pos[2].y += 2 * BLOCK_SIZE;
      pos[3].x -= 2 * BLOCK_SIZE;
    }
  else if (game->falling->type == 5) { // t.jpg
    if (game->falling->angle == 0)
      if (clockwise == 0) {
        pos[0].x += BLOCK_SIZE;
	pos[0].y -= BLOCK_SIZE;
      }
      else {
        pos[2].x -= BLOCK_SIZE;
	pos[2].y -= BLOCK_SIZE;
      }
    else if (game->falling->angle == 1)
      if (clockwise == 0) {
        pos[2].x += BLOCK_SIZE;
	pos[2].y += BLOCK_SIZE;
      }
      else {
        pos[3].x += BLOCK_SIZE;
	pos[3].y -= BLOCK_SIZE;
      }
    else if (game->falling->angle == 2)
      if (clockwise == 0) {
        pos[3].x -= BLOCK_SIZE;
	pos[3].y += BLOCK_SIZE;
      }
      else {
        pos[0].x += BLOCK_SIZE;
	pos[0].y -= BLOCK_SIZE;
	pos[2].x += BLOCK_SIZE;
	pos[2].y += BLOCK_SIZE;
	pos[3].x -= BLOCK_SIZE;
	pos[3].y += BLOCK_SIZE;
      }
    else
      if (clockwise == 0) {
        pos[0].x -= BLOCK_SIZE;
	pos[0].y += BLOCK_SIZE;
	pos[2].x -= BLOCK_SIZE;
	pos[2].y -= BLOCK_SIZE;
	pos[3].x += BLOCK_SIZE;
	pos[3].y -= BLOCK_SIZE;
      }
      else {
        pos[0].x -= BLOCK_SIZE;
	pos[0].y += BLOCK_SIZE;
      }
  }
  else // z.jpg
    if (game->falling->angle == 0) {
      pos[2].x -= BLOCK_SIZE;
      pos[3].x -= BLOCK_SIZE;
      pos[3].y -= 2 * BLOCK_SIZE;
    }
    else {
      pos[2].x += BLOCK_SIZE;
      pos[3].x += BLOCK_SIZE;
      pos[3].y += 2 * BLOCK_SIZE;
    }
  if (flip_checking(pos) == 1) {
    if (game->falling->type == 1 || game->falling->type == 4 || game->falling->type == 6)
      game->falling->angle ^= 1;
    else
      game->falling->angle += (clockwise == 0) ? -1 : 1;
    if (game->falling->angle == -1)
      game->falling->angle = 3;
    if (game->falling->angle == 4)
      game->falling->angle = 0;
    game->falling->pos[0] = pos[0];
    game->falling->pos[1] = pos[1];
    game->falling->pos[2] = pos[2];
    game->falling->pos[3] = pos[3];
  }
}
