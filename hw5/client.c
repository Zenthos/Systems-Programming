#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include <netdb.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

// Dimensions for the drawn grid (should be GRIDSIZE * texture dimensions)
#define GRID_DRAW_WIDTH 640
#define GRID_DRAW_HEIGHT 640

#define WINDOW_WIDTH GRID_DRAW_WIDTH
#define WINDOW_HEIGHT (HEADER_HEIGHT + GRID_DRAW_HEIGHT)

// Header displays current score
#define HEADER_HEIGHT 50

// Number of cells vertically/horizontally in the grid
#define GRIDSIZE 10

// Socket Definitions
#define BUFSIZE 4096
#define SA_IN struct sockaddr_in
#define SA struct sockaddr

#define MAXPLAYERCOUNT 4

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    int x;
    int y;
    int score;
    int connfd;
    int id;
} Player;

typedef enum {
    TILE_GRASS,
    TILE_TOMATO
} TILETYPE;

TILETYPE grid[GRIDSIZE][GRIDSIZE];
Player players[MAXPLAYERCOUNT];

int level;
int player_id;
int numTomatoes;

bool shouldExit = false;

TTF_Font* font;

// The information struct recieved from server
typedef struct {
    int score;
    int level;
    int numTomatoes;
    Player players[MAXPLAYERCOUNT];
    TILETYPE grid[GRIDSIZE][GRIDSIZE];
} ServerPacket;

// The information struct the client sends to the server
typedef struct {
    int x;
    int y;
} ResponsePacket;

void initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int rv = IMG_Init(IMG_INIT_PNG);
    if ((rv & IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "Error initializing IMG: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "Error initializing TTF: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
}

void onMove(int clientfd, int x, int y)
{
    // The packet we send to the server
    ResponsePacket packet = { x, y };

    // Send move to the server
    write(clientfd, &packet, sizeof(packet));
}

void handleKeyDown(SDL_KeyboardEvent* event, int clientfd)
{
    // ignore repeat events if key is held down
    if (event->repeat)
        return;

    int index = -1;
    for (int i = 0; i < MAXPLAYERCOUNT; i++) {
        if (players[i].id == player_id)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
        return;

    if (event->keysym.scancode == SDL_SCANCODE_Q || event->keysym.scancode == SDL_SCANCODE_ESCAPE)
        shouldExit = true;

    if (event->keysym.scancode == SDL_SCANCODE_UP || event->keysym.scancode == SDL_SCANCODE_W)
        onMove(clientfd, players[index].x, players[index].y - 1);

    if (event->keysym.scancode == SDL_SCANCODE_DOWN || event->keysym.scancode == SDL_SCANCODE_S)
        onMove(clientfd, players[index].x, players[index].y + 1);

    if (event->keysym.scancode == SDL_SCANCODE_LEFT || event->keysym.scancode == SDL_SCANCODE_A)
        onMove(clientfd, players[index].x - 1, players[index].y);

    if (event->keysym.scancode == SDL_SCANCODE_RIGHT || event->keysym.scancode == SDL_SCANCODE_D)
        onMove(clientfd, players[index].x + 1, players[index].y);
}

void processInputs(int clientfd)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				shouldExit = true;
				break;

            case SDL_KEYDOWN:
                handleKeyDown(&event.key, clientfd);
				break;

			default:
				break;
		}
	}
}

void drawGrid(
    SDL_Renderer* renderer, 
    SDL_Texture* grassTexture, 
    SDL_Texture* tomatoTexture, 
    SDL_Texture* playerTexture,
    SDL_Texture* selfTexture
) {
    SDL_Rect dest;
    for (int i = 0; i < GRIDSIZE; i++) {
        for (int j = 0; j < GRIDSIZE; j++) {
            dest.x = 64 * i;
            dest.y = 64 * j + HEADER_HEIGHT;
            SDL_Texture* texture = (grid[i][j] == TILE_GRASS) ? grassTexture : tomatoTexture;
            SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, texture, NULL, &dest);
        }
    }

    for (int i = 0; i < MAXPLAYERCOUNT; i++) {
        Player current = players[i];

        // Render other players
        if (current.id != 0 && current.id != player_id) {
            dest.x = 64 * current.x;
            dest.y = 64 * current.y + HEADER_HEIGHT;
            SDL_QueryTexture(playerTexture, NULL, NULL, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, playerTexture, NULL, &dest);
        }

        // Render self differently from other players
        if (current.id == player_id) {
            dest.x = 64 * current.x;
            dest.y = 64 * current.y + HEADER_HEIGHT;
            SDL_PixelFormatEnum format = SDL_PIXELFORMAT_RGB565;
            SDL_QueryTexture(selfTexture, &format, NULL, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, selfTexture, NULL, &dest);
        }
    }
}

void drawUI(SDL_Renderer* renderer)
{
    SDL_Color white = {255, 255, 255};
    SDL_Color blue = {0, 0, 255};

    for (int i = 0; i < MAXPLAYERCOUNT; i++) {
        Player current = players[i];
        
        if (current.id != 0) {
            // largest score/level supported is 2147483647
            char scoreStr[18];
            sprintf(scoreStr, "Score: %d", current.score);

            SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreStr, current.id == player_id ? blue : white);
            SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);

            SDL_Rect scoreDest;
            TTF_SizeText(font, scoreStr, &scoreDest.w, &scoreDest.h);
            scoreDest.x = 0;
            scoreDest.y = i * scoreDest.h;

            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreDest);
            SDL_FreeSurface(scoreSurface);
            SDL_DestroyTexture(scoreTexture);
        }
    }

    char levelStr[18];
    sprintf(levelStr, "Level: %d", level);

    SDL_Surface* levelSurface = TTF_RenderText_Solid(font, levelStr, white);
    SDL_Texture* levelTexture = SDL_CreateTextureFromSurface(renderer, levelSurface);

    SDL_Rect levelDest;
    TTF_SizeText(font, levelStr, &levelDest.w, &levelDest.h);
    levelDest.x = GRID_DRAW_WIDTH - levelDest.w;
    levelDest.y = 0;

    SDL_RenderCopy(renderer, levelTexture, NULL, &levelDest);
    SDL_FreeSurface(levelSurface);
    SDL_DestroyTexture(levelTexture);
}

void update(ServerPacket* buffer)
{
    // Update game state
    level = buffer->level;
    numTomatoes = buffer->numTomatoes;

    // Update Players
    for(int i = 0; i < MAXPLAYERCOUNT; i++) {
        Player player = buffer->players[i];
        players[i].x = player.x;
        players[i].y = player.y;
        players[i].score = player.score;
        players[i].id = player.id;
    }

    // Update grid
    for(int row = 0; row < GRIDSIZE; row++) {
        for (int col = 0; col < GRIDSIZE; col++)
        {
            grid[row][col] = buffer->grid[row][col];
        }
    }
}

int main(int argc, char* argv[])
{
    char *host, *port;
    int clientfd;

    SA_IN server_addr;

    // Ensure Host and Port is obtained from cli arguments
    if (argc != 3)
    {
        printf("The client is used in the format: %s <host> <port>.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    host = argv[1];
    port = argv[2];

    // Establish a socket to connect to server with
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Failed to create client socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(SA_IN));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));

    // Convert host address into usable format
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) 
    {
        printf("Failed to translate text address.\n");
        exit(EXIT_FAILURE);
    }

    // Establish connection to host address
    if (connect(clientfd, (SA*)&server_addr, sizeof(server_addr)) < 0) 
    {
        printf("Failed to connect to server address, %s:%s.\n", host, port);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    level = 1;

    initSDL();

    font = TTF_OpenFont("resources/Burbank-Big-Condensed-Bold-Font.otf", HEADER_HEIGHT);
    if (font == NULL) {
        fprintf(stderr, "Error loading font: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
    
    SDL_Window* window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    if (window == NULL) {
        fprintf(stderr, "Error creating app window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL)
	{
		fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
	}

    SDL_Texture *grassTexture = IMG_LoadTexture(renderer, "resources/grass.png");
    SDL_Texture *tomatoTexture = IMG_LoadTexture(renderer, "resources/tomato.png");
    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "resources/player.png");
    SDL_Texture *selfTexture = IMG_LoadTexture(renderer, "resources/self.png");

    ServerPacket* buffer = malloc(sizeof(ServerPacket));

    // Initial Data
    int id = 0;
    read(clientfd, &id, sizeof(int));
    player_id = id;

    // main game loop
    while (!shouldExit) {
        // If server sends a message, recieve it here, but don't wait for a message
        ssize_t bytes_read = recv(clientfd, buffer, sizeof(ServerPacket), MSG_DONTWAIT);

        if (bytes_read > 0) {
            // Update game state
            printf("%ld, bytes read from sever\n", bytes_read);
            update(buffer);
            memset(buffer, 0, sizeof(ServerPacket));
        }

        SDL_SetRenderDrawColor(renderer, 0, 105, 6, 255);
        SDL_RenderClear(renderer);

        processInputs(clientfd);

        drawGrid(renderer, grassTexture, tomatoTexture, playerTexture, selfTexture);
        drawUI(renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); // 16 ms delay to limit display to 60 fps
    }

    // clean up everything
    close(clientfd);
    free(buffer);

    SDL_DestroyTexture(grassTexture);
    SDL_DestroyTexture(tomatoTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(selfTexture);

    TTF_CloseFont(font);
    TTF_Quit();

    IMG_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
