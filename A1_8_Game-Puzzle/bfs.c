#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "./TreeFoof/tree.h"
#include "./TreeQueue/treeQueue.h"

enum {
    UP = -3 ,
    LEFT = -1 ,
    RIGHT = 1 ,
    DOWN = 3
} POSITIONS_t ;

const int FACTORIALS[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320}; // 3shan 2ady unique ID l kol e7temal

// 3shan a compare 3add 2 states 
bool stateCmp(char* state , char* goal){
    for (int i = 0 ; i < 9 ; i++){
        if (state[i] != goal[i])
            return false ;
    }
    return true ;
}


// kont nasy eny mmkn a print string b %s bas 5las b2a katbt el function lol
void printState(char* state){
   printf("%s\n",state);
}


int findZeroInState(char* state){
    for (int i = 0 ; i < 9 ; i++){
        if ( state[i] == '0' )
        return i ;
    }
    return 10 ;
}

// b convert kol state l unique ID 
int state_to_id(char* state) {
    int id = 0;
    
    for (int i = 0; i < 9; i++) {
        int count = 0;
        
        // kam rakm as8r mn [i] we 3la his right el mfrood en ay rakm yb2a kol el as8ar mno 3la shemalo fa a higher value means 
        // worst solution aw bat7rk f etegah 8lat mmkn ast5dha m3 el A star 3mtn we ashoof hyb2a a7san mn el manhatan wla eh 
        for (int j = i + 1; j < 9; j++) {
            if (state[j] < state[i]) {
                count++;
            }
        }
        
        // badrb kol factorial fel remaianing positions 
        id += count * FACTORIALS[8 - i];
    }
    
    return id;
}


bool is_solvable(const char *state)
{
    if (state == NULL || strlen(state) != 9)
        return false;

    // Validate: every digit from 0 through 8 appears exactly once.
    bool seen[9] = {false};

    for (int i = 0; i < 9; i++) {
        if (state[i] < '0' || state[i] > '8')
            return false;

        int tile = state[i] - '0';

        if (seen[tile])
            return false;

        seen[tile] = true;
    }

    // Count inversions, excluding the blank.
    int inversions = 0;

    for (int i = 0; i < 9; i++) {
        if (state[i] == '0')
            continue;

        for (int j = i + 1; j < 9; j++) {
            if (state[j] == '0')
                continue;

            if (state[i] > state[j])
                inversions++;
        }
    }

    return inversions % 2 == 0;
}

bool puzzFbfs(Node *root, char* goal) {
    Queue queue;
    initQueue(&queue);

    Node *stateNow = root ;
    long states_invented[362880] = {0} ; // 9! = 362880

    
    states_invented[state_to_id(stateNow->state)] = 1 ; // When I explore a node I create X number of states to transistion from this node
                                                        // This array works by bitmasking so I dont have to search the full array in O(n) only o(1) 
    int exploredStates = 0 ;
    while (1){  

        int zeroIndex = findZeroInState(stateNow->state);

        // Expand The State 

        // well a2dar at7rk f 4 amaken Up ,down ,left , right f ana b compute shakl el array law et7rkt f kol el amaken deh 
        // then bat2ked en ay state gdida mat3mltsh abl keda law lesa novy ba7otha child lel parent state el ana feha delw2ty 
        if ((zeroIndex) < 6){
            char newState[10] = {0};
            strcpy(newState, stateNow->state);
            newState[zeroIndex] = stateNow->state[zeroIndex + DOWN] ;
            newState[zeroIndex + DOWN] = '0' ;
            int id = state_to_id(newState);

            if (!states_invented[id]) { 
                Node *child = addChild(stateNow, newState);
                if (!enqueue(&queue, child)) {
                    fprintf(stderr, "Search queue is full.\n");
                    return false;
                }
                states_invented[id] = 1;
            }
        }
        if ((zeroIndex % 3) < 2 ){
            char newState[10] = {0};
            strcpy(newState, stateNow->state);
            newState[zeroIndex] = stateNow->state[zeroIndex + RIGHT] ;
            newState[zeroIndex + RIGHT] = '0' ;

            int id = state_to_id(newState);

            if (!states_invented[id]) {
                Node *child = addChild(stateNow, newState);
                if (!enqueue(&queue, child)) {
                    fprintf(stderr, "Search queue is full.\n");
                    return false;
                }
                states_invented[id] = 1;
            }
        }
          
        if (zeroIndex >= 3){
            char newState[10] = {0};
            strcpy(newState, stateNow->state);
            newState[zeroIndex] = stateNow->state[zeroIndex + UP] ;
            newState[zeroIndex + UP] = '0' ;
            int id = state_to_id(newState);

            if (!states_invented[id]) {
                Node *child = addChild(stateNow, newState);
                if (!enqueue(&queue, child)) {
                    fprintf(stderr, "Search queue is full.\n");
                    return false;
                }
            states_invented[id] = 1;
            }
           
        }

        if ((zeroIndex % 3) > 0 ){
            char newState[10] = {0};
            strcpy(newState, stateNow->state);
            newState[zeroIndex] = stateNow->state[zeroIndex + LEFT] ;
            newState[zeroIndex + LEFT] = '0' ;

            int id = state_to_id(newState);

            if (!states_invented[id]) {
                Node *child = addChild(stateNow, newState);
                if (!enqueue(&queue, child)) {
                    fprintf(stderr, "Search queue is full.\n");
                    return false;
                }
            states_invented[id] = 1;
            }
        }
        
        // check the state 
        bool success = dequeue(&queue , &stateNow) ;   

        if (!success) {
            printf("Queue is empty. No more nodes to explore and could not find the Goal.\n");
            return false ;
        }
         
        // printState(stateNow->state); ---> uncomment to print the explored states 
        exploredStates++ ; // increment the number of explored states
        if (stateCmp(stateNow->state, goal)) {
            // Count the states in the parent chain.
            int count = 0;
            Node *cursor = stateNow;

            while (cursor != NULL) {
                count++;
                cursor = cursor->parent;
            }

            // Allocate an array of pointers to those nodes.
            Node **path = malloc(count * sizeof *path);

            if (path == NULL) {
                fprintf(stderr, "Could not allocate the path array.\n");
                return false;
            }

            // Store the route backward
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
                return false;
            }

            printf("Goal %s found!\n", goal);

            // Read backward 
            int move = 0;

            for (int i = count - 1; i >= 0; i--) {
                printf("Move Number %d is %s \n", move, path[i]->state);
                fprintf(file, "Move Number %d is %s\n", move, path[i]->state);
                move++;
            }

            printf("The total moves are %d\n", count - 1);
            printf("The total explored states are %d\n", exploredStates);

            fclose(file);
            free(path);

            return true;
        }
   
    }
}


int main(void)
{
    // timestamp for the begining of execution to measure the running time of the program
    clock_t start_time = clock();

    // char* initialState = "123456780" ;

    char initialState[10];

    printf("Please enter the initial state of the puzzle (9 digits, 0-8, where 0 represents the blank space): ");
    scanf("%9s", initialState);

    for (int i = 0; i < 9; i++) {
        if (initialState[i] < '0' || initialState[i] > '8') {
            printf("Invalid input. Please enter exactly 9 digits from 0 to 8.\n");
            return 1;
        }
    }

    for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 9; j++) {
            if (initialState[i] == initialState[j]) {
                printf("Invalid input. Each digit from 0 to 8 must appear exactly once.\n");
                return 1;
            }
        }
    }
    

    if (stateCmp(initialState , "012345678")) {
        printf("The Puzzle is solved already ") ;
    }
    else {
        if (is_solvable(initialState))
        { 
        Node *root = createNode(initialState , 1);
        puzzFbfs(root, "012345678"); 
        freeTree(root);
        }
        else {
        printf("This Puzzle Can't be solved \n ") ;
        }
    }
    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute.\n", time_taken);

    return 0;
}

/* 3shan matensash el commands kal 3ada 

python3 puzzle_viewer.py puzzle_states.txt

gcc -std=c11 -Wall -Wextra bfs.c TreeFoof/tree.c TreeQueue/treeQueue.c -o BFS

./BFS
*/
