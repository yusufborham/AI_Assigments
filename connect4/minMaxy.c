#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROWS 6
#define COLS 7

#define EMPTY ' '

#define XXXX 40
#define XXX_ 39
#define XX_X 37
#define X_XX 31
#define _XXX 13

#define OOOO 80
#define OOO_ 78
#define OO_O 74
#define O_OO 62
#define _OOO 26

void printBoard(char *board , int rows , int cols)
{
    printf("Current Board State:\n");
     for (int i = 0 ; i<rows*cols ; i++ ){
        if((i%cols)==0 && i!=0)
            printf("\n"); 
        printf("%c ", board[i]) ;
    }

}

void printTopRow(int *topRow , int cols)
{
    printf("Current Top Row State:\n");
     for (int i = 0 ; i<cols ; i++ ){
        printf("%d ", topRow[i]) ;
    }

}

void initializeTopRow(int *topRow , int cols)
{
    for (int i = 0 ; i<cols ; i++)
        topRow[i] = ROWS-1 ; // INITIALIZE THE TOP ROWS OF EACH COLUMN TO BE THE LAST ROW (ROWS-1) SINCE THE BOARD IS EMPTY
}

void initializeBoard(char *board , int rows , int cols)
{
    for (int i = 0 ; i<rows*cols ; i++)
        board[i] = EMPTY ; // INITIALIZE THE BOARD TO BE EMPTY
}

int getEquivID(char x){
    if (x == 'x')
        return 1 ;
    else if (x == 'o')
        return 2 ;
    else 
        return 0 ;
}

int getHighestTopFreeRow(const int *topRow){
    int min = topRow[0] ;
    for (int i = 1 ; i < COLS ; i++){
        if (topRow[i] < min)
            min = topRow[i] ;
    }
    return min ;
}

int min(int a, int b){
    return (a < b) ? a : b;
}
int max(int a, int b){
    return (a > b) ? a : b;
}

typedef struct {
    int connect4CountMax;
    int connect3CountMax;
    int connect4CountMin;
    int connect3CountMin;
    int maxFoursByColumn[COLS];
    int minFoursByColumn[COLS];
    int immediateGainMax;
    int immediateGainMin;
} ScanResult;

static void recordPlayableThree(int column, int *connect3Count,
                                int foursByColumn[COLS], int *immediateGain)
{
    // A playable gap is always at nextFreeRow[column].
    int foursFromDrop = ++foursByColumn[column];
    ++(*connect3Count);
    *immediateGain = max(*immediateGain, foursFromDrop);
}

static inline void scanWindow(const char *board, const int *nextFreeRow, int row,
                       int col, int rowStep, int colStep, ScanResult *result)
{
    int id = 0;
    int playableColumn = -1;

    for (int i = 0; i < 4; ++i) {
        int cellRow = row + i * rowStep;
        int cellCol = col + i * colStep;
        int digit = getEquivID(board[cellRow * COLS + cellCol]);

        id = 3 * id + digit;
        if (digit == 0 && cellRow == nextFreeRow[cellCol]) {
            playableColumn = cellCol;
        }
    }

    switch (id) {
    case XXXX:
        ++result->connect4CountMax;
        break;

    case OOOO:
        ++result->connect4CountMin;
        break;

    case XXX_:
    case XX_X:
    case X_XX:
    case _XXX:
        if (playableColumn >= 0) {
            recordPlayableThree(playableColumn, &result->connect3CountMax,
                                result->maxFoursByColumn, &result->immediateGainMax);
        }
        break;

    case OOO_:
    case OO_O:
    case O_OO:
    case _OOO:
        if (playableColumn >= 0) {
            recordPlayableThree(playableColumn, &result->connect3CountMin,
                                result->minFoursByColumn, &result->immediateGainMin);
        }
        break;
    }
}

void scanHorizontal(const char *board, const int *nextFreeRow, ScanResult *result)
{
    int highestTopFreeRow = getHighestTopFreeRow(nextFreeRow);

    for (int row = ROWS - 1; row > highestTopFreeRow; --row) {
        for (int col = 0; col <= COLS - 4; ++col) {
            scanWindow(board, nextFreeRow, row, col, 0, 1, result);
        }
    }
}

void scanVertical(const char *board, const int *nextFreeRow, ScanResult *result)
{
    for (int col = 0; col < COLS; ++col) {
        int firstBottomRow = max(nextFreeRow[col] + 3, 3);

        for (int row = ROWS - 1; row >= firstBottomRow; --row) {
            scanWindow(board, nextFreeRow, row, col, -1, 0, result);
        }
    }
}

void scanDiagonal(const char *board, const int *nextFreeRow, ScanResult *result)
{
    for (int row = 0; row <= ROWS - 4; ++row) {
        for (int col = 0; col <= COLS - 4; ++col) {
            scanWindow(board, nextFreeRow, row, col, 1, 1, result);
            scanWindow(board, nextFreeRow, row, col + 3, 1, -1, result);
        }
    }
}

ScanResult evaluateBoard(const char *board, const int *nextFreeRow) // function to evaluate the board and return the scan result
{
    ScanResult result = {0};

    scanHorizontal(board, nextFreeRow, &result);
    scanVertical(board, nextFreeRow, &result);
    scanDiagonal(board, nextFreeRow, &result);

    return result;
}

int getImmediateGain(const ScanResult *result, bool maxToMove)  // function to get the immediate gain 
// imediate gain is the maximum number of connect fours that can be achieved by the next move
{
    return maxToMove ? result->immediateGainMax : -result->immediateGainMin;
}


int getScore(const ScanResult *result, bool maxToMove) // just criteria for the scoring could be changed at any time 
{
    int score = 10 * (result->connect4CountMax - result->connect4CountMin)
              +  5 * getImmediateGain(result, maxToMove)
              +      (result->connect3CountMax - result->connect3CountMin);
    return score;
}

int getUtility(const char *board , const int *nextFreeRow, bool maxToMove) // a lumped function that gives us the score of a given board
{
    ScanResult result = evaluateBoard(board, nextFreeRow);
    int score = getScore(&result, maxToMove);
    return score;
}


static const struct {
    const char *name;
    char rows[ROWS][COLS + 1];
} scanTestBoards[] = {
    {"Empty board", {
        ".......", ".......", ".......",
        ".......", ".......", "......."
    }},
    {"Playable horizontal X gap", {
        ".......", ".......", ".......",
        ".......", ".......", "xx.x.oo"
    }},
    {"Floating X gap", {
        ".......", ".......", ".......",
        ".......", "xx.x...", "oo.o..x"
    }},
    {"After O fills the lower gap", {
        ".......", ".......", ".......",
        ".......", "xx.x...", "oooo..x"
    }},
    {"Vertical fours and threes", {
        ".......", ".......", "x......",
        "x.....o", "x.....o", "x.....o"
    }},
    {"Overlapping fours and a full column", {
        "o......", "o......", "o......",
        "o......", "o......", "xxxxx.."
    }},
    {"Down-right X four", {
        ".......", ".......", ".x.....",
        ".oxoo..", ".xoxo.x", "oxoox.x"
    }},
    {"Down-left O four", {
        ".......", "...x...", "...o.o.",
        "...xoo.", "..xoxx.", "x.ooxxo"
    }},
    {"Playable threes on both slopes", {
        ".......", ".......", "...x..o",
        "...o..o", "...xoxx", "x.ooxox"
    }},
};

static void loadScanTestBoard(const char rows[ROWS][COLS + 1],
                              char *board, int *nextFreeRow)
{
    initializeTopRow(nextFreeRow, COLS);

    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            char cell = rows[row][col];
            board[row * COLS + col] = cell == '.' ? EMPTY : cell;
            if (cell != '.') {
                --nextFreeRow[col];
            }
        }
    }
}

void runScanTests(void)
{
    size_t testCount = sizeof scanTestBoards / sizeof scanTestBoards[0];
    // Keep repeated evaluations observable in an optimized benchmark.
    ScanResult (*volatile evaluate)(const char *, const int *) = evaluateBoard;

    for (size_t test = 0; test < testCount; ++test) {
        char board[ROWS * COLS];
        int nextFreeRow[COLS];

        loadScanTestBoard(scanTestBoards[test].rows, board, nextFreeRow);
        ScanResult result = evaluate(board, nextFreeRow);
        // print the score 
        int score = getScore(&result, true);
        printf("Test %zu - %s\n", test + 1, scanTestBoards[test].name);
        printBoard(board, ROWS, COLS);
        printf("\n");
        printf("Score: %d\n", score);

    }

}

void addDisk(char *board , int row , int col , bool player ){
    if (player)
        board[row*COLS + col] = 'x' ;
    else
        board[row*COLS + col] = 'o' ;
}

void removeDisk(char *board , int row , int col){
    board[row*COLS + col] = EMPTY ;
}

bool isBoardFull(int *nextFreeRow){
    for (int i = 0 ; i < COLS ; i++){
        if (nextFreeRow[i] >= 0)
            return false ;
    }
    return true ;
}

// if max is true, it means that the next move is for the maximizing player (X), otherwise it's for the minimizing player (O)
int minmax(char *board, int *nextFreeRow, int playsleft , bool max , bool isRoot , int *bestCol){
    if (isBoardFull(nextFreeRow) || playsleft == 0){
        int score = getUtility(board, nextFreeRow, max);
        return score;
    }
    int BestScore ;
    if (max)
        BestScore = -1000000 ;
    else 
        BestScore = 1000000 ;
    
    // loop on all the legal columns only
    for (int col = 0 ; col < COLS ; col++){
        if (nextFreeRow[col] < 0 )
            continue;
        int row = nextFreeRow[col] ;
        addDisk(board , row , col , max);
        nextFreeRow[col] = row -1  ;
        int childScore = minmax(board , nextFreeRow , playsleft -1 , !max , false , bestCol);
        removeDisk(board , row , col);
        nextFreeRow[col] = row ;

        if (max && childScore > BestScore){
            BestScore = childScore ;
            if (isRoot){
                *bestCol = col ; // update the best column to play for the maximizing player
            }
        }
            
        else if (!max && childScore < BestScore)
            BestScore = childScore ;
        
    }
    return BestScore ;
        
}

int main(void)
{
   
    clock_t start_time = clock();

    char board[ROWS * COLS];
    int nextFreeRow[COLS];

    initializeBoard(board, ROWS, COLS);
    initializeTopRow(nextFreeRow, COLS);

    for (int turns = 0 ; turns < 21 ; turns++){
        int bestCol = -1;
        int playsleft = 8; // Set the desired depth for the minimax search
        int score = minmax(board, nextFreeRow, playsleft, true, true, &bestCol);
        addDisk(board, nextFreeRow[bestCol], bestCol, true);
        nextFreeRow[bestCol]--;
        printBoard(board, ROWS, COLS);
        printf("\n");
        int opponentCol = -1 ;
        printf("Enter opponent's move (0-6): ");
        scanf("%d", &opponentCol);
        if (opponentCol < 0 || opponentCol >= COLS || nextFreeRow[opponentCol] < 0){
            printf("Invalid move by opponent. Exiting.\n");
            return EXIT_FAILURE;
        }
        addDisk(board, nextFreeRow[opponentCol], opponentCol, false);
        nextFreeRow[opponentCol]--;
        printBoard(board, ROWS, COLS);
        printf("\n");
    }

    printf("end of the game the winner is \n");
    int finalScore = getUtility(board, nextFreeRow, true);
    if (finalScore > 0)
        printf("X wins!\n");
    else if (finalScore < 0)
        printf("O wins!\n");
    else
        printf("It's a draw!\n");
    


    clock_t end_time = clock();

   
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute.\n", time_taken);

    return EXIT_SUCCESS;
}
