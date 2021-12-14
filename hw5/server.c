#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>

#define BUFSIZE 4096
#define SA_IN struct sockaddr_in
#define SA struct sockaddr

#define GRIDSIZE 10
#define MAXPLAYERCOUNT 4

// Game State
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
int numTomatoes;
int numPlayers;
pthread_mutex_t lock;

// The information struct received from a client
typedef struct {
    int x;
    int y;
} ClientPacket;

// The information struct that is sent to a client
typedef struct {
    int score;
    int level;
    int numTomatoes;
    Player players[MAXPLAYERCOUNT];
    TILETYPE grid[GRIDSIZE][GRIDSIZE];
} ResponsePacket;

// Game State Functions
double rand01();
void initGrid();
void initPlayers();
void emitAll();
void movePlayer(Player *player, int x, int y);

// Socket Functions
void* handle_connection(void* p_connfd);

int main(int argc, char *argv[])
{
    int listenfd, connfd, addr_size, port;
    SA_IN server_addr, client_addr;
    time_t t;

    char client_hostname[BUFSIZE], client_port[BUFSIZE];
    socklen_t clientlen = sizeof(struct sockaddr_storage);
    srand((unsigned) time(&t));

    // Ensure a port is obtained from cli arguments
    if (argc != 2)
    {
        printf("This server is used in the format: %s <port>.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[1]);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Failed to create server socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(SA_IN));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // This allows us to resuse existing open sockets
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)1, sizeof(int));

    if (bind(listenfd, (SA*)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("Failed to bind server socket to address.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 10) == -1)
    {
        printf("Failed to make server socket begin listening.\n");
        exit(EXIT_FAILURE);
    }

    // Mutex used to ensure thread synchronization 
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Failed to initialize Mutex.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize game state
    initPlayers();
    initGrid();

    printf("Server is listening on port: %d\n", port);
    while(true)
    {
        addr_size = sizeof(SA);
        connfd = accept(
            listenfd, 
            (SA*)&client_addr,
            (socklen_t*)&addr_size
        );

        if (connfd == -1)
        {
            printf("Failed to accept an incoming connection.\n");
            break;
        }

        if (numPlayers + 1 > MAXPLAYERCOUNT) 
        {
            printf("The server is full, please wait for an opening...");
            break;
        }

        // Print information about client connection
        getnameinfo((SA*)&client_addr, clientlen, client_hostname, BUFSIZE, client_port, BUFSIZE, 0);
        printf("Client connected from (%s, %s)\n", client_hostname, client_port);

        // Create new thread upon connection
        pthread_t thread;
        players[numPlayers].connfd = connfd;
        pthread_create(&thread, NULL, handle_connection, &players[numPlayers]);
    }

    close(listenfd);
    return 0;
}

void emitAll()
{
    ResponsePacket packet;
    packet.level = level;
    memcpy(packet.players, players, sizeof(players));
    memcpy(packet.grid, grid, sizeof(grid));

    // Send response packet to all clients
    for (int i = 0; i < numPlayers; i++) {
        write(players[i].connfd, &packet, sizeof(packet));
    }
}

void* handle_connection(void* p_player)
{
    Player *player = (Player*)p_player;
    int connfd = player->connfd;

    // Send player id to new player first
    int id = (int)pthread_self();
    write(connfd, &id, sizeof(int));
    player->id = id;
    numPlayers++;

    // Then update all clients to include new player
    emitAll();

    ClientPacket* buffer = malloc(sizeof(ClientPacket));

    while (true)
    {
        // Read incoming message from client, store in buffer
        ssize_t bytes_read = read(connfd, buffer, BUFSIZE - 1);

        if (bytes_read == 0) {
            // Client sent no data
            printf("A client has disconnected.\n");
            break;
        } else {
            printf("%ld, bytes were read, x: %d, y: %d\n", bytes_read, buffer->x, buffer->y);

            // Move Player
            pthread_mutex_lock(&lock);
            movePlayer(player, buffer->x, buffer->y);
            pthread_mutex_unlock(&lock);

            // Send the move to all clients regardless if move was successful
            emitAll();
        }
    }

    // Cleanup player on disconnect
    player->id = 0;
    player->score = 0;
    numPlayers--;

    // Send packet to all clients updating disconnecting player
    emitAll();

    // Close connection and thread
    free(buffer);
    close(connfd);
    return NULL;
}

// Game State Functions

double rand01()
{
    // get a random value in the range [0, 1]
    return (double) rand() / (double) RAND_MAX;
}

void initPlayers()
{
    for (int i = 0; i < MAXPLAYERCOUNT; i++)
    {
        // Ensures no two players will spawn on the same column
        players[i].x = (i * 2) + (rand() % 2);
        players[i].y = rand() % GRIDSIZE;
    }
}

void initGrid()
{
    for (int i = 0; i < GRIDSIZE; i++) {
        for (int j = 0; j < GRIDSIZE; j++) {
            double r = rand01();
            if (r < 0.1) {
                grid[i][j] = TILE_TOMATO;
                numTomatoes++;
            }
            else
                grid[i][j] = TILE_GRASS;
        }
    }

    // force player's position to be grass
    for (int i = 0; i < numPlayers; i++) {
      Player current = players[i];

      if (grid[current.x][current.y] == TILE_TOMATO) {
          grid[current.x][current.y] = TILE_GRASS;
          numTomatoes--;
      }
    }

    // ensure grid isn't empty
    while (numTomatoes == 0)
        initGrid();
}

void movePlayer(Player *player, int x, int y)
{
    // Prevent falling off the grid
    if (x < 0 || x >= GRIDSIZE || y < 0 || y >= GRIDSIZE)
        return;

    // Player can only move to 4 adjacent squares
    if (!(abs(player->x - x) == 1 && abs(player->y - y) == 0) &&
        !(abs(player->x - x) == 0 && abs(player->y - y) == 1)) {
        printf("Invalid move attempted from (%d, %d) to (%d, %d)\n", 
            player->x, player->y, x, y);
        return;
    }

    // Player cannot move on top of other players
    for (int i = 0; i < MAXPLAYERCOUNT; i++) {
        Player current = players[i];

        if (current.x == x && current.y == y && current.id != 0) {
            printf("Invalid move attempted from (%d, %d) to (%d, %d)\n", 
                player->x, player->y, x, y);
            return;
        }
    }

    player->x = x;
    player->y = y;

    if (grid[x][y] == TILE_TOMATO) {
        grid[x][y] = TILE_GRASS;
        player->score++;
        numTomatoes--;
        if (numTomatoes == 0) {
            level++;
            initGrid();
        }
    }
}