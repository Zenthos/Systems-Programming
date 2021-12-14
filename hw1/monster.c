#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Player {
    int x;
    int y;
};

struct Monster {
    int x;
    int y;
};

struct Board {
    int width;
    int height;
    int goalX;
    int goalY;
};

int movePlayer(struct Player *p, struct Board *b, char input) {
    if (input == 'N' && p->y > 0)
	p->y = p->y - 1;
    else if (input == 'S' && p->y < b->height - 1)
	p->y = p->y + 1;
    else if (input == 'E' && p->x < b->width - 1)
	p->x = p->x + 1;
    else if (input == 'W' && p->x > 0)
	p->x = p->x - 1;
    else {
	printf("invalid move\n");
	return 0;
    }

    return 1;
}

void moveMonster(struct Player *p, struct Monster *m) {
    int xDistance = abs(p->x - m->x);
    int yDistance = abs(p->y - m->y);

    if (xDistance > yDistance) {
	if (p->x > m->x) {
	    m->x = m->x + 1;
            printf("monster moves E \n");
	} else {
	    m->x = m->x - 1;
	    printf("monster moves W \n");
	}
    } else {
	if (p->y > m->y) {
	    m->y = m->y + 1;
	    printf("monster moves S \n");
	} else {
	    m->y = m->y - 1;
	    printf("monster moves N \n");
	}
    }
}

void printBoard(struct Board b, struct Monster m, struct Player p) {
    for (int y = 0; y < b.height; y++) {
	for (int x = 0; x < b.width; x++) {
	    if (y == m.y && x == m.x)
		printf("M ");
	    else if (y == p.y && x == p.x)
		printf("P ");
	    else if (x == b.goalX && y == b.goalY)
		printf("G ");
	    else
		printf(". ");
	}
	printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 9) {
	printf("Not enough arguments\n");
	return 1;
    }

    const int BOARDX = atoi(argv[1]);
    const int BOARDY = atoi(argv[2]);

    const int GOALX = atoi(argv[5]);
    const int GOALY = atoi(argv[6]);

    int plyrX = atoi(argv[3]);
    int plyrY = atoi(argv[4]);

    int monX = atoi(argv[7]);
    int monY = atoi(argv[8]);

    struct Board board = (struct Board){ BOARDX, BOARDY, GOALX, GOALY };
    struct Monster monster = (struct Monster){ monX, monY };
    struct Player player = (struct Player){ plyrX, plyrY };

    while (1) {
        printBoard(board, monster, player);

        char input[256];
	char *line = fgets(input, 256, stdin);

	if (line == NULL) {
	    printf("^D\n");
	    break;
	}

	int success = movePlayer(&player, &board, (char)input[0]);

	/* Check if player won */
	if (player.x == board.goalX && player.y == board.goalY) {
	   printf("player wins!\n");
	   break;
	}

	/* Move monster if player has not won */
	if (success == 1) {
	    moveMonster(&player, &monster);
	}

	/* Check if player lost */
	if (player.x == monster.x && player.y == monster.y) {
	   printf("monster wins!\n");
	   break;
	}
    }

    return 0;
}
