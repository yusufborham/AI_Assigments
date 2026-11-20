#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROWS 4
#define COLS 4

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

#include "./TreeFoof/tree.h"
// #include "./TreePriorityQueue/treePriorityQueue.h"


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

int getHighestTopFreeRow(int *topRow){
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

void scanHorizontal(char *board, int *nextFreeRow, int *connect4CountMax, int *connect3CountMax, int *connect4CountMin, int *connect3CountMin) {
    int highestTopFreeRow = getHighestTopFreeRow(nextFreeRow); // the minimum top row of all columns to know when to stop scanning the board

    for (int rowCursor = ROWS - 1; rowCursor > highestTopFreeRow; rowCursor--) {
        for (int startcursor = 0; startcursor <= COLS - 4; startcursor++) {
            int ID = 0;
            for (int i = 0; i < 4; i++) {
                ID = 3 * ID + getEquivID(board[COLS * rowCursor + startcursor + i]);
            }
            bool is_1_playable = rowCursor == nextFreeRow[startcursor + 0];
            bool is_2_playable = rowCursor == nextFreeRow[startcursor + 1];
            bool is_3_playable = rowCursor == nextFreeRow[startcursor + 2];
            bool is_4_playable = rowCursor == nextFreeRow[startcursor + 3];

            if (ID == XXXX)
                (*connect4CountMax)++;
            else if ((ID == XXX_) && is_4_playable)
                (*connect3CountMax)++;
            else if ((ID == XX_X) && is_3_playable)
                (*connect3CountMax)++;
            else if ((ID == X_XX) && is_2_playable)
                (*connect3CountMax)++;
            else if ((ID == _XXX) && is_1_playable)
                (*connect3CountMax)++;
            ///////////////////////////////////////////////////
            else if (ID == OOOO)
                (*connect4CountMin)++;
            else if ((ID == OOO_) && is_4_playable)
                (*connect3CountMin)++;
            else if ((ID == OO_O) && is_3_playable)
                (*connect3CountMin)++;
            else if ((ID == O_OO) && is_2_playable)
                (*connect3CountMin)++;
            else if ((ID == _OOO) && is_1_playable)
                (*connect3CountMin)++;
        }
    }
}

void scanVertical(char *board, int *nextFreeRow, int *connect4CountMax, int *connect3CountMax, int *connect4CountMin, int *connect3CountMin) {
    for (int colCursor = 0; colCursor < COLS; colCursor++) {
        for (int startcursor = ROWS - 1; startcursor >= (max(3 + nextFreeRow[colCursor],3)) ; startcursor--){ // so dont waste time and check 
                                                                                                        //the full column / law h detect ba3d keda 
                                                                                                        // 2 disks 5aly el 3 hena 2 
            int ID = 0;
            for (int i = 0; i < 4; i++) {
                ID = 3 * ID + getEquivID(board[COLS * (startcursor-i) + colCursor]); // awal element fel ID howa el ta7t
            }                    
            if (ID == XXXX)
                (*connect4CountMax)++;
            else if ((ID == XXX_) ){
                (*connect3CountMax)++;
                printf("he he %d %d \n" , colCursor , startcursor) ;
            }
                           
            ///////////////////////////////////////////////////
            else if (ID == OOOO)
                (*connect4CountMin)++;
            else if ((ID == OOO_) )
                (*connect3CountMin)++;                                                                        
        }
    }
}

void scanDiagonal(char *board, int *nextFreeRow, int *connect4CountMax, int *connect3CountMax, int *connect4CountMin, int *connect3CountMin) {
    //scanning diagonals from down left to up right 
    for(int rowCursor = 3 ; rowCursor <= ROWS-1 ; rowCursor++){
        for(int windows = 0 ; windows < rowCursor-2 ; windows++){
            int ID = 0;
            for (int i = 0; i < 4; i++) {
                ID = 3 * ID + getEquivID(board[COLS * (rowCursor - windows - i) + (i + windows)]); // awel element howa bedayet el diagonal mn down left
            }  
            bool is_1_playable = (rowCursor - windows - 0) == nextFreeRow[windows + 0];
            bool is_2_playable = (rowCursor - windows - 1) == nextFreeRow[windows + 1];
            bool is_3_playable = (rowCursor - windows - 2) == nextFreeRow[windows + 2];
            bool is_4_playable = (rowCursor - windows - 3) == nextFreeRow[windows + 3];

            if (ID == XXXX)
                (*connect4CountMax)++;
            else if ((ID == XXX_) && is_4_playable)
                (*connect3CountMax)++;
            else if ((ID == XX_X) && is_3_playable)
                (*connect3CountMax)++;
            else if ((ID == X_XX) && is_2_playable)
                (*connect3CountMax)++;
            else if ((ID == _XXX) && is_1_playable)
                (*connect3CountMax)++;
            ///////////////////////////////////////////////////
            else if (ID == OOOO)
                (*connect4CountMin)++;
            else if ((ID == OOO_) && is_4_playable)
                (*connect3CountMin)++;
            else if ((ID == OO_O) && is_3_playable)
                (*connect3CountMin)++;
            else if ((ID == O_OO) && is_2_playable)
                (*connect3CountMin)++;
            else if ((ID == _OOO) && is_1_playable)
                (*connect3CountMin)++;
        }
    }
    // scanning from upper right to down left 
    for(int rowCursor = ROWS - 4  ;  rowCursor >= 0 ; rowCursor--){
        for (int windows = 0 ; windows <= (ROWS - 4 - rowCursor) ; windows++){
            int ID = 0;
            for (int i = 0; i < 4; i++) {
                ID = 3 * ID + getEquivID(board[COLS * (rowCursor + windows + i) + (COLS - 1 - i - windows)]); // awel element howa bedayet el diagonal mn up right
            }  
            bool is_1_playable = (rowCursor + windows + 0) == nextFreeRow[COLS - 1 - windows - 0];
            bool is_2_playable = (rowCursor + windows + 1) == nextFreeRow[COLS - 1 - windows - 1];
            bool is_3_playable = (rowCursor + windows + 2) == nextFreeRow[COLS - 1 - windows - 2];
            bool is_4_playable = (rowCursor + windows + 3) == nextFreeRow[COLS - 1 - windows - 3];

            if (ID == XXXX)
                (*connect4CountMax)++;
            else if ((ID == XXX_) && is_4_playable)
                (*connect3CountMax)++;
            else if ((ID == XX_X) && is_3_playable)
                (*connect3CountMax)++;
            else if ((ID == X_XX) && is_2_playable)
                (*connect3CountMax)++;
            else if ((ID == _XXX) && is_1_playable)
                (*connect3CountMax)++;
            ///////////////////////////////////////////////////
            else if (ID == OOOO)
                (*connect4CountMin)++;
            else if ((ID == OOO_) && is_4_playable)
                (*connect3CountMin)++;
            else if ((ID == OO_O) && is_3_playable)
                (*connect3CountMin)++;
            else if ((ID == O_OO) && is_2_playable)
                (*connect3CountMin)++;
            else if ((ID == _OOO) && is_1_playable)
                (*connect3CountMin)++;
        }
    }

    // scanning from up right to down left

    for (int rowCursor = ROWS - 4 ; rowCursor >= 0; rowCursor--){
        for ( int windows = 0 ; windows <= (ROWS - 4 - rowCursor) ; windows++){
            int ID = 0;
            for (int i = 0; i < 4; i++) {
                ID = 3 * ID + getEquivID(board[COLS * (rowCursor + windows + i) + (i + windows)]); // awel element howa bedayet el diagonal mn up left
            }       

            bool is_1_playable = (rowCursor + windows + 0) == nextFreeRow[windows + 0];
            bool is_2_playable = (rowCursor + windows + 1) == nextFreeRow[windows + 1];
            bool is_3_playable = (rowCursor + windows + 2) == nextFreeRow[windows + 2];
            bool is_4_playable = (rowCursor + windows + 3) == nextFreeRow[windows + 3];

            if (ID == XXXX)
                (*connect4CountMax)++;
            else if ((ID == XXX_) && is_4_playable)
                (*connect3CountMax)++;
            else if ((ID == XX_X) && is_3_playable)
                (*connect3CountMax)++;
            else if ((ID == X_XX) && is_2_playable)
                (*connect3CountMax)++;
            else if ((ID == _XXX) && is_1_playable)
                (*connect3CountMax)++;
            ///////////////////////////////////////////////////
            else if (ID == OOOO)
                (*connect4CountMin)++;
            else if ((ID == OOO_) && is_4_playable)
                (*connect3CountMin)++;
            else if ((ID == OO_O) && is_3_playable)
                (*connect3CountMin)++;
            else if ((ID == O_OO) && is_2_playable)
                (*connect3CountMin)++;
            else if ((ID == _OOO) && is_1_playable)
                (*connect3CountMin)++;  
        }
    }

    // scanning from down left to up right
    for (int rowCursor = 3 ; rowCursor <= ROWS - 1; rowCursor++){
        for (int windows = 0 ; windows < rowCursor - 2 ; windows++){
            int ID = 0;
            for (int i = 0; i < 4; i++) {
                ID = 3 * ID + getEquivID(board[COLS * (rowCursor - windows - i) + (COLS - 1 - i - windows)]); // awel element howa bedayet el diagonal mn down right
            }   

            bool is_1_playable = (rowCursor - windows - 0) == nextFreeRow[COLS - 1 - windows - 0];
            bool is_2_playable = (rowCursor - windows - 1) == nextFreeRow[COLS - 1 - windows - 1];
            bool is_3_playable = (rowCursor - windows - 2) == nextFreeRow[COLS - 1 - windows - 2];
            bool is_4_playable = (rowCursor - windows - 3) == nextFreeRow[COLS - 1 - windows - 3];      

            if (ID == XXXX)
                (*connect4CountMax)++;
            else if ((ID == XXX_) && is_4_playable)
                (*connect3CountMax)++;
            else if ((ID == XX_X) && is_3_playable)
                (*connect3CountMax)++;
            else if ((ID == X_XX) && is_2_playable)
                (*connect3CountMax)++;
            else if ((ID == _XXX) && is_1_playable)
                (*connect3CountMax)++;
            ///////////////////////////////////////////////////
            else if (ID == OOOO)
                (*connect4CountMin)++;
            else if ((ID == OOO_) && is_4_playable)
                (*connect3CountMin)++;
            else if ((ID == OO_O) && is_3_playable)
                (*connect3CountMin)++;
            else if ((ID == O_OO) && is_2_playable)
                (*connect3CountMin)++;
            else if ((ID == _OOO) && is_1_playable)
                (*connect3CountMin)++;
        }
    }
}



int main(void)
{
    // timestamp for the begining of execution to measure the running time of the program
    clock_t start_time = clock();


    char board[ROWS * COLS] = {0} ; 
    int  nextFreeRow[COLS] = {0} ;       // to keep track of the top row of each column in the board

    initializeTopRow(nextFreeRow, COLS);
    initializeBoard(board, ROWS, COLS);

    // fill out the connect board with a full finshed game state
    board[0]= 'o';
    board[1]= 'o';
    board[2]= EMPTY;
    board[3]= EMPTY;
    //
    board[4]= 'x';
    board[5]= 'o';
    board[6]= 'x';
    board[7]= 'x';
    //
    board[8]= 'x';
    board[9]= 'x';
    board[10]= 'x';
    board[11]= 'o';
    //
    board[12]= 'x';
    board[13]= 'x';
    board[14]= 'x';     
    board[15]= 'x';

    printBoard(board , ROWS , COLS) ;   
    printf("\n\n");

    nextFreeRow[0] = -1 ; 
    nextFreeRow[1] = -1;
    nextFreeRow[2] = 0;
    nextFreeRow[3] = 0 ;

    printTopRow(nextFreeRow, COLS);
    printf("\n\n");

    // next move (rowCursor - windows)

    // scanning 

    
    int connect4CountMax = 0 ; 
    int connect3CountMax = 0 ; 

    int connect4CountMin = 0 ; 
    int connect3CountMin = 0 ; 

    scanHorizontal(board, nextFreeRow, &connect4CountMax, &connect3CountMax, &connect4CountMin, &connect3CountMin);
    scanVertical(board, nextFreeRow, &connect4CountMax, &connect3CountMax, &connect4CountMin, &connect3CountMin);
    scanDiagonal(board, nextFreeRow, &connect4CountMax, &connect3CountMax, &connect4CountMin, &connect3CountMin);
    

    printf("connect4CountMax: %d\n", connect4CountMax);
    printf("connect3CountMax: %d\n", connect3CountMax);
    printf("connect4CountMin: %d\n", connect4CountMin);
    printf("connect3CountMin: %d\n", connect3CountMin);

    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("The program took %f milliseconds to execute.\n", time_taken * 1000);

    return 0;
}


