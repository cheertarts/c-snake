
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>

/* Game-wide macros */
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 600
#define CHUNK_SIZE 10
/* Snake Macros */
#define SNAKE_COLOR_R 255
#define SNAKE_COLOR_G 255
#define SNAKE_COLOR_B 255
#define SNAKE_SPEED 10
#define SNAKE_TAIL_INCREMENT 5
/* Apple Macros */
#define APPLE_COLOR_R 255
#define APPLE_COLOR_G 0
#define APPLE_COLOR_B 255

struct _Apple {
    int x;
    int y;
};

struct _SnakeChunk {
    int x;
    int y;
};

struct _Snake {
    int x_velocity;
    int y_velocity;
    struct _SnakeChunk head;
    struct {
        unsigned int chunks_to_add;
        size_t capacity;
        size_t length;
        struct _SnakeChunk *array;
    } tail;
};

typedef struct _Apple Apple;
typedef struct _SnakeChunk SnakeChunk;
typedef struct _Snake Snake;
typedef enum {false, true} bool;

static unsigned int score = 0;

static void print_sdl_error(const char * const function_name) {
    fprintf(stderr, "%s Error: %s\n", function_name, SDL_GetError());
}

static void begin_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        print_sdl_error("SDL_Init");
        exit(1);
    }
}

static SDL_Window *create_sdl_window() {
    SDL_Window *window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (window == NULL) {
        print_sdl_error("SDL_CreateWindow");
        SDL_Quit();
        exit(1);
    }
    return window;
}

static SDL_Renderer *create_sdl_renderer(SDL_Window *window) {
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
                                                SDL_RENDERER_ACCELERATED |
                                                SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        print_sdl_error("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    return renderer;
}

static void destroy_sdl(SDL_Window *window, SDL_Renderer *renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void move_apple(Apple * const apple) {
    size_t size = CHUNK_SIZE;
    apple->x = (rand() % (int)floor(((WINDOW_WIDTH - 1) / size))) * size;
    apple->y = (rand() % (int)floor(((WINDOW_HEIGHT - 1) / size))) * size;
}

static void draw_apple(const Apple * const apple,
                       SDL_Renderer * const renderer) {
    SDL_Rect rect = {apple->x, apple->y, CHUNK_SIZE, CHUNK_SIZE};
    SDL_SetRenderDrawColor(renderer, APPLE_COLOR_R, APPLE_COLOR_G,
                           APPLE_COLOR_B, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &rect);
}

static void create_snake(Snake * const snake) {
    const size_t size = CHUNK_SIZE;
    snake->x_velocity = 0;
    snake->y_velocity = SNAKE_SPEED;
    /* create head */
    snake->head.x = floor((WINDOW_WIDTH / 2 - size / 2) / size) * size;
    snake->head.y = floor((WINDOW_HEIGHT / 2 - size / 2) / size) * size;
    /* create tail */
    snake->tail.chunks_to_add = SNAKE_TAIL_INCREMENT - 1;
    snake->tail.capacity = SNAKE_TAIL_INCREMENT;
    snake->tail.length = 0;
    snake->tail.array = malloc(snake->tail.capacity * sizeof(SnakeChunk));
    if (snake->tail.array == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(1);
    }
}

static void destroy_snake(Snake * const snake) {
    free(snake->tail.array);
    snake->tail.array = NULL;
    snake->tail.capacity = 0;
    snake->tail.length = 0;
}

static void update_snake(Snake * const snake, Apple * const apple) {
    int i;
    /* create new tail chunks */
    if (snake->tail.chunks_to_add > 0) {
        if (snake->tail.length >= snake->tail.capacity) {
            snake->tail.capacity *= 2;
            snake->tail.array = realloc(snake->tail.array,
                                        snake->tail.capacity *
                                        sizeof(SnakeChunk));
            if (snake->tail.array == NULL) {
                fprintf(stderr, "Failed to allocate memory.\n");
                exit(1);
            }
        }
        snake->tail.length++;
        snake->tail.chunks_to_add--;
    }
    /* update the snake tail's x and y values */
    for (i = snake->tail.length - 1; i >= 1; --i) {
        snake->tail.array[i].x = snake->tail.array[i - 1].x;
        snake->tail.array[i].y = snake->tail.array[i - 1].y;
    }
    if (snake->tail.length >= 1) {
        snake->tail.array[0].x = snake->head.x;
        snake->tail.array[0].y = snake->head.y;
    }
    /* set the snake head's x and y values */
    snake->head.x += snake->x_velocity;
    snake->head.y += snake->y_velocity;
    if (snake->head.x < 0) {
        snake->head.x = WINDOW_WIDTH - 1;
    } else if (snake->head.x > WINDOW_WIDTH - 1) {
        snake->head.x = 0;
    }
    if (snake->head.y < 0) {
        snake->head.y = WINDOW_HEIGHT - 1;
    } else if (snake->head.y > WINDOW_HEIGHT - 1) {
        snake->head.y = 0;
    }
    /* check if the snake hit itself */
    for (i = 0; i < snake->tail.length; ++i) {
        if (snake->head.x == snake->tail.array[i].x &&
            snake->head.y == snake->tail.array[i].y) {
            destroy_snake(snake);
            create_snake(snake);
            move_apple(apple);
            score = 0;
        }
    }
    /* check if the snake's head is at the apple */
    if (snake->head.x >= apple->x - CHUNK_SIZE + 1 &&
        snake->head.y >= apple->y - CHUNK_SIZE + 1 &&
        snake->head.x <= apple->x + CHUNK_SIZE - 1 &&
        snake->head.y <= apple->y + CHUNK_SIZE - 1) {
        snake->tail.chunks_to_add += SNAKE_TAIL_INCREMENT;
        score++;
        printf("Score: %d\n", score);
        move_apple(apple);
    }
}

static void draw_snake(const Snake * const snake,
                       SDL_Renderer * const renderer) {
    SDL_Rect chunk;
    unsigned int i;
    chunk.w = CHUNK_SIZE;
    chunk.h = CHUNK_SIZE;
    SDL_SetRenderDrawColor(renderer, SNAKE_COLOR_R, SNAKE_COLOR_G,
                           SNAKE_COLOR_B, SDL_ALPHA_OPAQUE);
    /* draw head */
    chunk.x = snake->head.x;
    chunk.y = snake->head.y;
    SDL_RenderFillRect(renderer, &chunk);
    /* draw tail */
    for (i = 0; i < snake->tail.length; ++i) {
        chunk.x = snake->tail.array[i].x;
        chunk.y = snake->tail.array[i].y;
        SDL_RenderFillRect(renderer, &chunk);
    }
}

static bool event_handling(SDL_Event * const e, Snake * const snake) {
    bool quit = false;
    while (SDL_PollEvent(e)) {
        if (e->type == SDL_QUIT) {
            quit = true;
        } else if (e->type == SDL_KEYDOWN) {
            switch (e->key.keysym.sym) {
            case SDLK_ESCAPE:
                quit = true;
                break;
            case SDLK_w:
            case SDLK_UP:
                snake->y_velocity = -SNAKE_SPEED;
                snake->x_velocity = 0;
                break;
            case SDLK_s:
            case SDLK_DOWN:
                snake->y_velocity = SNAKE_SPEED;
                snake->x_velocity = 0;
                break;
            case SDLK_a:
            case SDLK_LEFT:
                snake->y_velocity = 0;
                snake->x_velocity = -SNAKE_SPEED;
                break;
            case SDLK_d:
            case SDLK_RIGHT:
                snake->y_velocity = 0;
                snake->x_velocity = SNAKE_SPEED;
                break;
            }
        }
    }
    return quit;
}

static void game_loop(SDL_Renderer * const renderer, Snake * const snake,
                      Apple * const apple) {
    SDL_Rect rectangle = {100, 100, 30, 30};
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        quit = event_handling(&e, snake);
        update_snake(snake, apple);
        SDL_RenderClear(renderer);
        draw_snake(snake, renderer);
        draw_apple(apple, renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderPresent(renderer);
    }
}

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    begin_sdl();
    window = create_sdl_window();
    renderer = create_sdl_renderer(window);
    srand(time(NULL));
    {
        Snake snake;
        Apple apple;
        create_snake(&snake);
        move_apple(&apple);
        game_loop(renderer, &snake, &apple);
        destroy_snake(&snake);
    }
    destroy_sdl(window, renderer);
    return 0;
}
