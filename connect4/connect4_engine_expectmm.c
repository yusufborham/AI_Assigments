#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

#define ROWS 6
#define COLS 7

#define EMPTY ' '

#define XXXX 40

#define XXX_ 39
#define XX_X 37
#define X_XX 31
#define _XXX 13

#define xx__  36  //
#define x_x_  30   
#define x__x  28   
#define _xx_  12   
#define _x_x  10   
#define __xx   4   

#define OOOO 80

#define OOO_ 78
#define OO_O 74
#define O_OO 62
#define _OOO 26

#define oo__  72   
#define o_o_  60   
#define o__o  56   
#define _oo_  24   
#define _o_o  20   
#define __oo   8  


typedef struct {
    char board[ROWS * COLS];  // Saved copy of the 42 cells
    int score;                // Result from getUtility()
    bool maxToMove;           // Whose turn it was on this board
    bool valid;               // Whether this slot contains a saved entry
} CacheEntry;

#define CACHE_SIZE 1000000
static CacheEntry cache[CACHE_SIZE];

static unsigned cacheIndex(const char *board)
{
    uint64_t hash = 14695981039346656037ULL;

    for (int i = 0; i < ROWS * COLS; ++i) {
        hash ^= (unsigned char)board[i];
        hash *= 1099511628211ULL;
    }

    return (unsigned)(hash % CACHE_SIZE);
}

void printBoard(char *board, int rows, int cols)
{
    puts("Current Board State:");

    printf("  ");
    for (int col = 0; col < cols; ++col) {
        printf("%-4d", col);
    }
    putchar('\n');

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            printf("+---");
        }
        puts("+");

        for (int col = 0; col < cols; ++col) {
            printf("| %c ", board[row * cols + col]);
        }
        puts("|");
    }

    for (int col = 0; col < cols; ++col) {
        printf("+---");
    }
    puts("+");
}

void printBoardFile(FILE *file, const char *board, int rows, int cols)
{
    fprintf(file, "Current Board State:\n");

    fprintf(file, "  ");
    for (int col = 0; col < cols; ++col) {
        fprintf(file, "%-4d", col);
    }
    fputc('\n', file);

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            fprintf(file, "+---");
        }
        fprintf(file, "+\n");

        for (int col = 0; col < cols; ++col) {
            fprintf(file, "| %c ", board[row * cols + col]);
        }
        fprintf(file, "|\n");
    }

    for (int col = 0; col < cols; ++col) {
        fprintf(file, "+---");
    }
    fprintf(file, "+\n");
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
    int connect2CountMax;
    int connect2CountMin;
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

static void recordPlayableTwo(int *connect2Count)
{
    ++(*connect2Count);
}

static inline void scanWindow(const char *board, const int *nextFreeRow, int row,
                       int col, int rowStep, int colStep, ScanResult *result , bool isVerticalScan)
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

    case xx__:
        if (!isVerticalScan) {
            if ((row + 2 * rowStep) == nextFreeRow[col + 2 * colStep] &&
                (row + 3 * rowStep) == nextFreeRow[col + 3 * colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        } else {
            if ((row + 2 * rowStep) == nextFreeRow[col + 2 * colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        }
        break;

    case x_x_:
        if (!isVerticalScan) {
            if ((row + rowStep) == nextFreeRow[col + colStep] &&
                (row + 3 * rowStep) == nextFreeRow[col + 3 * colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        }
        break;

    case x__x:
        if (!isVerticalScan) {
            if ((row + rowStep) == nextFreeRow[col + colStep] &&
                (row + 2 * rowStep) == nextFreeRow[col + 2 * colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        }
        break;

    case _xx_:
        if (!isVerticalScan) {
            if (row == nextFreeRow[col] &&
                (row + 3 * rowStep) == nextFreeRow[col + 3 * colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        }
        break;

    case _x_x:
        if (!isVerticalScan) {
            if (row == nextFreeRow[col] &&
                (row + 2 * rowStep) == nextFreeRow[col + 2 * colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        }
        break;

    case __xx:
        if (!isVerticalScan) {
            if (row == nextFreeRow[col] &&
                (row + rowStep) == nextFreeRow[col + colStep]) {
                recordPlayableTwo(&result->connect2CountMax);
            }
        }
        break;

    case OOO_:
    case OO_O:
    case O_OO:
    case _OOO:
        if (playableColumn >= 0) {
            recordPlayableThree(playableColumn ,&result->connect3CountMin,
                                result->minFoursByColumn, &result->immediateGainMin);
        }
        break;

    case oo__:
        if (!isVerticalScan) {
            if ((row + 2 * rowStep) == nextFreeRow[col + 2 * colStep] &&
                (row + 3 * rowStep) == nextFreeRow[col + 3 * colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        } else {
            if ((row + 2 * rowStep) == nextFreeRow[col + 2 * colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        }
        break;

    case o_o_:
        if (!isVerticalScan) {
            if ((row + rowStep) == nextFreeRow[col + colStep] &&
                (row + 3 * rowStep) == nextFreeRow[col + 3 * colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        }
        break;

    case o__o:
        if (!isVerticalScan) {
            if ((row + rowStep) == nextFreeRow[col + colStep] &&
                (row + 2 * rowStep) == nextFreeRow[col + 2 * colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        }
        break;

    case _oo_:
        if (!isVerticalScan) {
            if (row == nextFreeRow[col] &&
                (row + 3 * rowStep) == nextFreeRow[col + 3 * colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        }
        break;

    case _o_o:
        if (!isVerticalScan) {
            if (row == nextFreeRow[col] &&
                (row + 2 * rowStep) == nextFreeRow[col + 2 * colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        }
        break;

    case __oo:
        if (!isVerticalScan) {
            if (row == nextFreeRow[col] &&
                (row + rowStep) == nextFreeRow[col + colStep]) {
                recordPlayableTwo(&result->connect2CountMin);
            }
        }
        break;
    }
}

void scanHorizontal(const char *board, const int *nextFreeRow, ScanResult *result)
{
    int highestTopFreeRow = getHighestTopFreeRow(nextFreeRow);

    for (int row = ROWS - 1; row > highestTopFreeRow; --row) {
        for (int col = 0; col <= COLS - 4; ++col) {
            scanWindow(board, nextFreeRow, row, col, 0, 1, result, false);
        }
    }
}

void scanVertical(const char *board, const int *nextFreeRow, ScanResult *result)
{
    for (int col = 0; col < COLS; ++col) {
        int firstBottomRow = max(nextFreeRow[col] + 2, 3); // change el +2 le +3 law hat hat3mel detection le 3 bas mesh 2 kman

        for (int row = ROWS - 1; row >= firstBottomRow; --row) {
            scanWindow(board, nextFreeRow, row, col, -1, 0, result, true);
        }
    }
}

void scanDiagonal(const char *board, const int *nextFreeRow, ScanResult *result)
{
    for (int row = 0; row <= ROWS - 4; ++row) {
        for (int col = 0; col <= COLS - 4; ++col) {
            scanWindow(board, nextFreeRow, row, col, 1, 1, result , false);
            scanWindow(board, nextFreeRow, row, col + 3, 1, -1, result , false);
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
    score = 139*score + (result->connect2CountMax - result->connect2CountMin) ; // remove el line dah law hashil el 2's men el 7esba
    return score;
}

int getUtility(const char *board , const int *nextFreeRow, bool maxToMove) // a lumped function that gives us the score of a given board
{
    ScanResult result = evaluateBoard(board, nextFreeRow);
    int score = getScore(&result, maxToMove);
    return score;
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
int minmax(char *board, int *nextFreeRow, int playsleft , bool max , bool isRoot , int *bestCol , int *colOrder , int alpha , int beta , bool useAlphaBeta , FILE *file){
    // fprintf(file , "LAYER NUMBER %d \n" , 4 - playsleft );
    // printBoardFile(file , board , ROWS , COLS);
    if (playsleft == 0 || isBoardFull(nextFreeRow)) {
        CacheEntry *entry = &cache[cacheIndex(board)];

        if (entry->valid && entry->maxToMove == max &&
            memcmp(entry->board, board, ROWS * COLS) == 0) {
            return entry->score;
        }

        int score = getUtility(board, nextFreeRow, max);

        // fprintf(file , "The Score Here Is %d \n" , score);

        memcpy(entry->board, board, ROWS * COLS);
        entry->maxToMove = max;
        entry->score = score;
        entry->valid = true;

        return score;
    }
    int BestScore ;
    if (max)
        BestScore = -1000000 ;
    else 
        BestScore = 1000000 ;
    
    // loop on all the legal columns only
    for (int i = 0 ; i < COLS ; i++){
        int col = colOrder[i];
        if (nextFreeRow[col] < 0 ) // mafish mkan fel column dah 
            continue;
        int row = nextFreeRow[col] ; 
        addDisk(board , row , col , max);
        nextFreeRow[col] = row -1  ;
        int childScore = minmax(board , nextFreeRow , playsleft -1 , !max , false , bestCol , colOrder , alpha , beta , useAlphaBeta , file);
        removeDisk(board , row , col);
        nextFreeRow[col] = row ;

           
        if (max) {
            if (childScore > BestScore) {
                BestScore = childScore;
                if (isRoot) {
                    *bestCol = col;
                }
            }
            if (useAlphaBeta && (BestScore > alpha) ) {
                alpha = BestScore;
            }
        } else {
            if (childScore < BestScore) {
                BestScore = childScore;
            }
            if (useAlphaBeta && (BestScore < beta)) {
                beta = BestScore;
            }
        }

        if (useAlphaBeta && (alpha >= beta) ) {
            break;
        }
                
    }
    return BestScore ;
                
}

typedef struct {
    double score;
    char board[ROWS * COLS];
    int playsleft;
    bool maxToMove;
    bool valid;
} ExpectCacheEntry;

static ExpectCacheEntry expectCache[CACHE_SIZE];

// Weights are in fifths, and a lone available neighbor gets both shares.
static int getSideWeight(const int *nextFreeRow, int col, int otherCol)
{
    if (col < 0 || col >= COLS || nextFreeRow[col] < 0)
        return 0;
    return (otherCol >= 0 && otherCol < COLS && nextFreeRow[otherCol] >= 0) ? 1 : 2;
}

static int getDropColumn(const int *nextFreeRow, int col)
{
    int left = getSideWeight(nextFreeRow, col - 1, col + 1);
    int right = getSideWeight(nextFreeRow, col + 1, col - 1);
    int roll;
    do {
        roll = rand();
    } while (roll >= RAND_MAX - RAND_MAX % 5);
    roll %= 5;

    if (roll < left)
        return col - 1;
    if (roll >= 5 - right)
        return col + 1;
    return col;
}

static double expectiminimax(char *board, int *nextFreeRow, int playsleft , bool max ,
                             int *bestCol , int *colOrder , double alpha , double beta , bool useAlphaBeta)
{
    ExpectCacheEntry *entry = &expectCache[cacheIndex(board)];
    if (!bestCol && entry->valid && entry->playsleft == playsleft && entry->maxToMove == max &&
        memcmp(entry->board, board, ROWS * COLS) == 0) {
        return entry->score;
    }

    double originalAlpha = alpha;
    double originalBeta = beta;
    double BestScore = max ? INT_MIN : INT_MAX;
    if (playsleft == 0 || isBoardFull(nextFreeRow)) {
        BestScore = getUtility(board, nextFreeRow, max);
    } else {
        for (int i = 0 ; i < COLS ; i++) {
            int col = colOrder[i];
            if (nextFreeRow[col] < 0)
                continue;

            int left = getSideWeight(nextFreeRow, col - 1, col + 1);
            int right = getSideWeight(nextFreeRow, col + 1, col - 1);
            int weights[] = {left, 5 - left - right, right};
            bool certain = left + right == 0;
            double childScore = 0;
            for (int j = 0 ; j < 3 ; j++) {
                if (weights[j] == 0)
                    continue;
                int dropCol = col + j - 1;
                int row = nextFreeRow[dropCol];
                addDisk(board , row , dropCol , max);
                nextFreeRow[dropCol] = row - 1;
                // Stochastic outcomes need exact values before taking their average.
                double score = expectiminimax(board , nextFreeRow , playsleft - 1 , !max , NULL , colOrder ,
                                               certain ? alpha : INT_MIN , certain ? beta : INT_MAX , useAlphaBeta);
                removeDisk(board , row , dropCol);
                nextFreeRow[dropCol] = row;
                childScore += certain ? score : weights[j] * score;
            }
            if (!certain)
                childScore /= 5;

            if (max ? childScore > BestScore : childScore < BestScore) {
                BestScore = childScore;
                if (bestCol)
                    *bestCol = col;
            }
            if (useAlphaBeta) {
                if (max && BestScore > alpha)
                    alpha = BestScore;
                if (!max && BestScore < beta)
                    beta = BestScore;
                if (alpha >= beta)
                    break;
            }
        }
    }

    // A search outside its original window may have returned only a bound.
    if (BestScore > originalAlpha && BestScore < originalBeta) {
        memcpy(entry->board, board, ROWS * COLS);
        entry->playsleft = playsleft;
        entry->maxToMove = max;
        entry->score = BestScore;
        entry->valid = true;
    }
    return BestScore;
}

// mesh ana el kateb el 7eta deeh 
/* The Python GUI sends a column number and receives one line per board update. */
static void sendGuiState(const char *board, const int *nextFreeRow,
                         char nextPlayer, int lastColumn)
{
    ScanResult result = evaluateBoard(board, nextFreeRow);

    printf("STATE %c %d %d %d ", nextPlayer, lastColumn,
           result.connect4CountMax, result.connect4CountMin);
    for (int i = 0; i < ROWS * COLS; ++i) {
        putchar(board[i] == EMPTY ? '.' : board[i]);
    }
    putchar('\n');
    fflush(stdout);
}
// le7ad hena dah code lel GUI for sending dataaaa 

static int runGui(int depth , bool useAlphaBeta , FILE *file)
{
    char board[ROWS * COLS];
    int nextFreeRow[COLS];
    int colOrder[COLS] = {3, 2, 4, 1, 5, 0, 6};
    int playsleft = depth; // Same depth as the console game

    initializeBoard(board, ROWS, COLS);
    initializeTopRow(nextFreeRow, COLS);
    srand((unsigned)time(NULL));

    printf("READY %d %d\n", ROWS, COLS);
    sendGuiState(board, nextFreeRow, 'x', -1);

    // fprintf(file, "READY \n");
    int reply = 1 ;

    while (!isBoardFull(nextFreeRow)) {
        int bestCol = -1;
        // fprintf(file ,"REPLY NUMBER %d \n" ,reply++);
        expectiminimax(board, nextFreeRow, playsleft, true, &bestCol, colOrder , INT_MIN , INT_MAX , useAlphaBeta);
        if (bestCol < 0 || bestCol >= COLS || nextFreeRow[bestCol] < 0) {
            fputs("Expectiminimax did not return a legal column.\n", stderr);
            return EXIT_FAILURE;
        }

        bestCol = getDropColumn(nextFreeRow, bestCol);
        addDisk(board, nextFreeRow[bestCol], bestCol, true);
        nextFreeRow[bestCol]--;
        sendGuiState(board, nextFreeRow, 'o', bestCol);

        char line[64];
        int opponentCol;
        for (;;) {
            if (fgets(line, sizeof line, stdin) == NULL) {
                return EXIT_SUCCESS;
            }

            char *end;
            long col = strtol(line, &end, 10);
            if (end != line && end[strspn(end, " \t\r\n")] == '\0' &&
                col >= 0 && col < COLS && nextFreeRow[col] >= 0) {
                opponentCol = (int)col;
                break;
            }
            puts("INVALID");
            fflush(stdout);
        }

        opponentCol = getDropColumn(nextFreeRow, opponentCol);
        addDisk(board, nextFreeRow[opponentCol], opponentCol, false);
        nextFreeRow[opponentCol]--;
        sendGuiState(board, nextFreeRow, isBoardFull(nextFreeRow) ? '-' : 'x',opponentCol);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{   
    FILE *file = fopen("GameTree.txt", "w");
    

    if (argc > 1 && strcmp(argv[1], "--gui") == 0) {
        if (argc == 2) {
            return runGui(8, true , file);
        }

        if (argc != 4) {
            fputs("Usage: connect4_engine --gui [depth 0|1]\n", stderr);
            return EXIT_FAILURE;
        }

        char *end;
        long depth = strtol(argv[2], &end, 10);
        if (*end != '\0' || depth < 1 || depth > ROWS * COLS ||
            (strcmp(argv[3], "0") != 0 && strcmp(argv[3], "1") != 0)) {
            fputs("Invalid depth or alpha-beta flag.\n", stderr);
            return EXIT_FAILURE;
        }


        return runGui((int)depth, strcmp(argv[3], "1") == 0 , file);
        fclose(file);
    }

   

    return EXIT_SUCCESS;
}
