#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "./TreeFoof/tree.h"
#include "./TreeStack/stacktree.h"

enum {
    UP = -3,
    LEFT = -1,
    RIGHT = 1,
    DOWN = 3
};

typedef enum {
    SEARCH_ERROR = -1,
    SEARCH_NOT_FOUND = 0,
    SEARCH_FOUND = 1
} SearchResult;

const int FACTORIALS[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320};

bool stateCmp(const char *state, const char *goal)
{
    for (int i = 0; i < 9; i++) {
        if (state[i] != goal[i])
            return false;
    }
    return true;
}

int findZeroInState(const char *state)
{
    for (int i = 0; i < 9; i++) {
        if (state[i] == '0')
            return i;
    }
    return -1;
}

/* Permutation rank: an identifier, not a distance heuristic. */
int state_to_id(const char *state)
{
    int id = 0;
    for (int i = 0; i < 9; i++) {
        int count = 0;
        for (int j = i + 1; j < 9; j++) {
            if (state[j] < state[i])
                count++;
        }
        id += count * FACTORIALS[8 - i];
    }
    return id;
}

/* Validation and parity test for the fixed goal 012345678. */
bool is_solvable(const char *state)
{
    if (state == NULL || strlen(state) != 9)
        return false;

    bool seen[9] = {false};
    for (int i = 0; i < 9; i++) {
        if (state[i] < '0' || state[i] > '8')
            return false;
        int tile = state[i] - '0';
        if (seen[tile])
            return false;
        seen[tile] = true;
    }

    int inversions = 0;
    for (int i = 0; i < 9; i++) {
        if (state[i] == '0')
            continue;
        for (int j = i + 1; j < 9; j++) {
            if (state[j] != '0' && state[i] > state[j])
                inversions++;
        }
    }
    return inversions % 2 == 0;
}

SearchResult puzzFds(Node *root, const char *goal, int limit)
{
    if (root == NULL || goal == NULL || limit < 0) {
        fprintf(stderr, "Invalid search arguments.\n");
        return SEARCH_ERROR;
    }

    Stack stack;
    initStack(&stack);

    Node *stateNow = root;
    root->cost = 0;

    /* Reset this table for each depth-limited attempt. */
    int bestDepth[362880];
    for (int i = 0; i < 362880; i++)
        bestDepth[i] = INT_MAX;
    bestDepth[state_to_id(root->state)] = 0;

    int exploredStates = 0;

    while (1) {
        /* Test before expansion, including the root and depth-limit nodes. */
        if (stateCmp(stateNow->state, goal)) {
            int count = 0;
            Node *cursor = stateNow;
            while (cursor != NULL) {
                count++;
                cursor = cursor->parent;
            }

            Node **path = malloc((size_t)count * sizeof *path);
            if (path == NULL) {
                fprintf(stderr, "Could not allocate the path array.\n");
                return SEARCH_ERROR;
            }

            cursor = stateNow;
            int index = 0;
            while (cursor != NULL) {
                path[index++] = cursor;
                cursor = cursor->parent;
            }

            FILE *file = fopen("puzzle_states.txt", "w");
            if (file == NULL) {
                perror("Could not open puzzle_states.txt");
                free(path);
                return SEARCH_ERROR;
            }

            printf("Goal %s found!\n", goal);
            int move = 0;
            for (int i = count - 1; i >= 0; i--) {
                printf("Move Number %d is %s\n", move, path[i]->state);
                fprintf(file, "Move Number %d is %s\n", move, path[i]->state);
                move++;
            }

            printf("The total moves are %d\n", count - 1);
            printf("The total cost is %d\n", stateNow->cost);
            printf("Expanded states in the final iteration: %d\n", exploredStates);
            fclose(file);
            free(path);
            return SEARCH_FOUND;
        }

        int zeroIndex = findZeroInState(stateNow->state);
        if (zeroIndex < 0) {
            fprintf(stderr, "A board has no blank tile.\n");
            return SEARCH_ERROR;
        }

        /* cost is the depth of THIS node, not the number of expansions. */
        if (stateNow->cost < limit) {
            exploredStates++;

            if (zeroIndex < 6) {
                char newState[10];
                strcpy(newState, stateNow->state);
                newState[zeroIndex] = stateNow->state[zeroIndex + DOWN];
                newState[zeroIndex + DOWN] = '0';
                int id = state_to_id(newState);
                int nextDepth = stateNow->cost + 1;

                if (nextDepth < bestDepth[id]) {
                    Node *child = addChild(stateNow, newState);
                    child->cost = nextDepth;
                    if (!push(&stack, child)) {
                        fprintf(stderr, "Search stack is full at depth limit %d.\n", limit);
                        return SEARCH_ERROR;
                    }
                    bestDepth[id] = nextDepth;
                }
            }

            if (zeroIndex % 3 < 2) {
                char newState[10];
                strcpy(newState, stateNow->state);
                newState[zeroIndex] = stateNow->state[zeroIndex + RIGHT];
                newState[zeroIndex + RIGHT] = '0';
                int id = state_to_id(newState);
                int nextDepth = stateNow->cost + 1;

                if (nextDepth < bestDepth[id]) {
                    Node *child = addChild(stateNow, newState);
                    child->cost = nextDepth;
                    if (!push(&stack, child)) {
                        fprintf(stderr, "Search stack is full at depth limit %d.\n", limit);
                        return SEARCH_ERROR;
                    }
                    bestDepth[id] = nextDepth;
                }
            }

            if (zeroIndex >= 3) {
                char newState[10];
                strcpy(newState, stateNow->state);
                newState[zeroIndex] = stateNow->state[zeroIndex + UP];
                newState[zeroIndex + UP] = '0';
                int id = state_to_id(newState);
                int nextDepth = stateNow->cost + 1;

                if (nextDepth < bestDepth[id]) {
                    Node *child = addChild(stateNow, newState);
                    child->cost = nextDepth;
                    if (!push(&stack, child)) {
                        fprintf(stderr, "Search stack is full at depth limit %d.\n", limit);
                        return SEARCH_ERROR;
                    }
                    bestDepth[id] = nextDepth;
                }
            }

            if (zeroIndex % 3 > 0) {
                char newState[10];
                strcpy(newState, stateNow->state);
                newState[zeroIndex] = stateNow->state[zeroIndex + LEFT];
                newState[zeroIndex + LEFT] = '0';
                int id = state_to_id(newState);
                int nextDepth = stateNow->cost + 1;

                if (nextDepth < bestDepth[id]) {
                    Node *child = addChild(stateNow, newState);
                    child->cost = nextDepth;
                    if (!push(&stack, child)) {
                        fprintf(stderr, "Search stack is full at depth limit %d.\n", limit);
                        return SEARCH_ERROR;
                    }
                    bestDepth[id] = nextDepth;
                }
            }
        }

        /* Skip entries superseded by a shallower discovery. */
        Node *next;
        while (1) {
            if (!pop(&stack, &next))
                return SEARCH_NOT_FOUND;  /* Not found within THIS limit. */

            int id = state_to_id(next->state);
            if (next->cost == bestDepth[id]) {
                stateNow = next;
                break;
            }
        }
    }
}

int main(void)
{
    clock_t start_time = clock();
    char initialState[] = "123456780";
    const char *goal = "012345678";

    if (!is_solvable(initialState)) {
        fprintf(stderr, "The starting board is invalid or unsolvable for this goal.\n");
        return EXIT_FAILURE;
    }

    SearchResult result = SEARCH_NOT_FOUND;
    int layer;

    /* For this 3x3 puzzle, the maximum shortest-solution length is 31. */
    for (layer = 0; layer <= 31; layer++) {
        printf("Trying depth limit %d\n", layer);
        Node *root = createNode(initialState, 1);
        result = puzzFds(root, goal, layer);
        freeTree(root);

        if (result == SEARCH_ERROR)
            return EXIT_FAILURE;
        if (result == SEARCH_FOUND)
            break;
    }

    if (result != SEARCH_FOUND) {
        fprintf(stderr, "No solution found within 31 moves. Check the search and helper functions.\n");
        return EXIT_FAILURE;
    }

    double time_taken = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    printf("The program took %f seconds of CPU time.\n", time_taken);
    printf("The successful depth limit is %d\n", layer);
    return EXIT_SUCCESS;
}

/*
gcc -std=c11 -Wall -Wextra iddfs.c TreeFoof/tree.c TreeStack/stacktree.c -o IDDFS
./IDDFS
python3 puzzle_viewer.py puzzle_states.txt
*/
