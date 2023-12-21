#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <time.h>

#define N 200

const char *title = "pelletman";
const int width = 800;
const int height = 600;

int game = 1;
int score = 0;

int dirx = 1; // Look, it makes perfect sense to store the player-direction in a global variable
int diry = 0;
float speed = 1.5f;

int pellet_count = N;
int active_pellet = 0;

enum {
    UP,
    DOWN,
    LEFT,
    RIGHT
} typedef direction;

struct {
    /* 
    * From the start, the segment(s) were stored as an array of SDL_Rect. But through a lot of iteration,
    * and suffering, the code became shit. 
    * But it is funny, that no other struct in the player array, stores the length of the player. lol
    * I was going to change this but so much of the code hinges on this now that I can't be bothered.
    */
    int id;
    float posx;
    float posy;
    int character_size; // Ranges from 5 to around 30
    float oldpos[2];
    int length;
    direction dir;
    SDL_Rect segment;
} typedef player;

struct {
    int id;
    int pellet_size;
    bool active;
    SDL_Rect rect;
} typedef pellet;
/* 
pellet generate_pellet() {
    srand(time(NULL));
    return (pellet) {0, 1, (SDL_Rect) {rand() % (width - 10), rand() % (height - 10), 10, 10}};
}
*/
void generate_pellets(pellet *pellets) {
    srand(time(NULL));
    pellets[0].id = 0;
    pellets[0].active = true;
    pellets[0].pellet_size = 15;
    pellets[0].rect = (SDL_Rect) {rand() % (width - pellets[0].pellet_size), rand() % (height - pellets[0].pellet_size), pellets[0].pellet_size, pellets[0].pellet_size};
    for (int i = 1; i < N; i++) {
        pellets[i].id = i;
        pellets[i].active = false;
        pellets[i].pellet_size = 15;
        pellets[i].rect = (SDL_Rect) {rand() % (width - pellets[i].pellet_size), rand() % (height - pellets[i].pellet_size), pellets[i].pellet_size, pellets[i].pellet_size};
    }
}

void init_player(player *p) {
    p->id = 0;
    p->posx = 600;
    p->posy = 200;
    p->length = 1;
    p->dir = RIGHT;
    p->character_size = 15;
    p->segment = (SDL_Rect) {(width - p->character_size) / 2, (height - p->character_size) / 2, 10, 10};
}

void set_player_direction(direction dir, player *p) {
    if (p->length > 1) {
        for (int i = 0; i < p->length; i++) {
            p[i].dir = p[i-1].dir;
        }
    }
    switch (dir) {
        case UP:
            dirx = 0;
            diry = -1;
            p->dir = UP;
            break;
        case DOWN:
            dirx = 0;
            diry = 1;
            p->dir = DOWN;
            break;
        case LEFT:
            dirx = -1;
            diry = 0;
            p->dir = LEFT;
            break;
        case RIGHT:
            dirx = 1;
            diry = 0;
            p->dir = RIGHT;
            break;
    }
}

void spawn_segment(player *p) {
    switch ((p+p->length)->dir) {
        case UP:
            p->segment = (SDL_Rect) {(int) p->oldpos[0], (int) p->oldpos[1] - p->character_size, p->character_size, p->character_size};
            break;
        case DOWN:
            p->segment = (SDL_Rect) {(int) p->oldpos[0], (int) p->oldpos[1] + p->character_size, p->character_size, p->character_size};
            break;
        case LEFT:
            p->segment = (SDL_Rect) {(int) p->oldpos[0] + p->character_size, (int) p->oldpos[1], p->character_size, p->character_size};
            break;
        case RIGHT:
            p->segment = (SDL_Rect) {(int) p->oldpos[0] - p->character_size, (int) p->oldpos[1], p->character_size, p->character_size};
            break;
    }
}

void update_game(player *p1, pellet *pellets) {

    // pellet pel = generate_pellet(); Deprecated. So it stays.

    // Update player position
        for (int i = 0; i < p1->length; i++) {
            if (i > 0) {
                p1[i].oldpos[0] = p1[i].posx;
                p1[i].oldpos[1] = p1[i].posy;
                p1[i].posx = p1[i-1].oldpos[0];
                p1[i].posy = p1[i-1].oldpos[1];
                switch (p1[i].dir) {
                    case UP:
                        p1[i].segment.y = (int) p1[i].posy + p1->character_size;
                        break;
                    case DOWN:
                        p1[i].segment.y = (int) p1[i].posy - p1->character_size;
                        break;
                    case LEFT:
                        p1[i].segment.x = (int) p1[i].posx + p1->character_size;
                        break;
                    case RIGHT:
                        p1[i].segment.x = (int) p1[i].posx - p1->character_size;
                        break;
                }
                
            } else {
                p1[i].oldpos[0] = p1[i].posx;
                p1[i].oldpos[1] = p1[i].posy;
                p1[i].posx += dirx * speed;
                p1[i].posy += diry * speed;
                p1[i].segment.x = (int) p1[i].posx;
                p1[i].segment.y = (int) p1[i].posy;

            }
            p1[i].segment.w = p1->character_size;
            p1[i].segment.h = p1->character_size;
        }

        // Pellet collision and a lot of other shit
        for (int i = 0; i < pellet_count; i++) {
            if (SDL_HasIntersection(&p1->segment, &pellets[i].rect) && pellets[i].active == true) {
                printf("Collision with pellet %d\n", pellets[i].id);
                // Memory management stuff. 
                memset(&pellets[i], 0, sizeof(pellet));
                if (i == pellet_count - 1) {
                    if (pellet_count == 0) free(pellets); // This never runs lolz!
                    else if (pellet_count == 1) {
                        pellets = realloc(pellets, sizeof(pellet) * 1);
                    } else {
                        pellets = realloc(pellets, sizeof(pellet) * (pellet_count - 1));
                    }
                } else {
                    memmove(&pellets[i], &pellets[i + 1], sizeof(pellet) * (pellet_count - i - 1));
                    pellets = realloc(pellets, sizeof(pellet) * (pellet_count - 1));
                }
                // Add a segment to the player
                
                spawn_segment(p1);
                p1->length++;
                // Disable old pellet and set new active pellet
                pellets[active_pellet].active = false;
                active_pellet++;
                pellets[active_pellet].active = true;
                score++;
                printf("Score: %d\n", score);
                pellet_count--;
                SDL_Delay(250);
            }
        }
}

void draw_game(SDL_Renderer *renderer, player *p1, pellet *pellets) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw the player
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < p1->length; i++) {
        SDL_RenderDrawRect(renderer, &p1[i].segment);
        SDL_RenderFillRect(renderer, &p1[i].segment);
    }

    // Draws the pellets
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    
    SDL_RenderDrawRect(renderer, &pellets[active_pellet].rect);
    SDL_RenderFillRect(renderer, &pellets[active_pellet].rect);

    // Shitty death animation
    if (game == 0) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Point point_arr[5];
        for (int i = 0; i < 5; i++) {
            SDL_Point point = {p1->segment.x + i * 5, p1->segment.y + i * 5};
            point_arr[i] = point;
        }
        SDL_RenderDrawLines(renderer, point_arr , 5);
        SDL_Delay(250);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    
    pellet *pellets = malloc(sizeof(pellet) * N);
    generate_pellets(pellets);
    
    player *p1 = malloc(sizeof(player) * N);
    init_player(p1);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_VideoInit(NULL);
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    while(game) {
        if (score == N) {
            printf("You win!\n");
            break;
        }
        // Event handling
        if (SDL_PollEvent(&event)) {
            // Player movement
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_w && p1[0].dir != DOWN) {
                    set_player_direction(UP, p1);
                }
                if (event.key.keysym.sym == SDLK_s && p1[0].dir != UP) {
                    set_player_direction(DOWN, p1);
                }
                if (event.key.keysym.sym == SDLK_a && p1[0].dir != RIGHT) {
                    set_player_direction(LEFT, p1);
                }
                if (event.key.keysym.sym == SDLK_d && p1[0].dir != LEFT) {
                    set_player_direction(RIGHT, p1);
                }
                // Quit
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    printf("Quitting...\n");
                    break;
                }
            }
            if (event.type == SDL_QUIT) {
                printf("Quitting...\n");
                break;
            }
        }
        
        update_game(p1, pellets);

        draw_game(renderer, p1, pellets);

        SDL_Delay(1000/240);
    }
    free(p1);
    free(pellets); // Runs here tho!
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_VideoQuit();
    SDL_Quit();
    return 0;
} 