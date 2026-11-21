void scanDiagonal(char *board, int *nextFreeRow, int *connect4CountMax, int *connect3CountMax, int *connect4CountMin, int *connect3CountMin) {
//     //scanning diagonals from down left to up right 
//     for(int rowCursor = 3 ; rowCursor <= ROWS-1 ; rowCursor++){
//         for(int windows = 0 ; windows < rowCursor-2 ; windows++){
//             int ID = 0;
//             for (int i = 0; i < 4; i++) {
//                 ID = 3 * ID + getEquivID(board[COLS * (rowCursor - windows - i) + (i + windows)]); // awel element howa bedayet el diagonal mn down left
//             }  
//             bool is_1_playable = (rowCursor - windows - 0) == nextFreeRow[windows + 0];
//             bool is_2_playable = (rowCursor - windows - 1) == nextFreeRow[windows + 1];
//             bool is_3_playable = (rowCursor - windows - 2) == nextFreeRow[windows + 2];
//             bool is_4_playable = (rowCursor - windows - 3) == nextFreeRow[windows + 3];

//             if (ID == XXXX)
//                 (*connect4CountMax)++;
//             else if ((ID == XXX_) && is_4_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == XX_X) && is_3_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == X_XX) && is_2_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == _XXX) && is_1_playable)
//                 (*connect3CountMax)++;
//             ///////////////////////////////////////////////////
//             else if (ID == OOOO)
//                 (*connect4CountMin)++;
//             else if ((ID == OOO_) && is_4_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == OO_O) && is_3_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == O_OO) && is_2_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == _OOO) && is_1_playable)
//                 (*connect3CountMin)++;
//         }
//     }
//     // scanning from upper right to down left 
//     for(int rowCursor = ROWS - 4  ;  rowCursor >= 0 ; rowCursor--){
//         for (int windows = 0 ; windows <= (ROWS - 4 - rowCursor) ; windows++){
//             int ID = 0;
//             for (int i = 0; i < 4; i++) {
//                 ID = 3 * ID + getEquivID(board[COLS * (rowCursor + windows + i) + (COLS - 1 - i - windows)]); // awel element howa bedayet el diagonal mn up right
//             }  
//             bool is_1_playable = (rowCursor + windows + 0) == nextFreeRow[COLS - 1 - windows - 0];
//             bool is_2_playable = (rowCursor + windows + 1) == nextFreeRow[COLS - 1 - windows - 1];
//             bool is_3_playable = (rowCursor + windows + 2) == nextFreeRow[COLS - 1 - windows - 2];
//             bool is_4_playable = (rowCursor + windows + 3) == nextFreeRow[COLS - 1 - windows - 3];

//             if (ID == XXXX)
//                 (*connect4CountMax)++;
//             else if ((ID == XXX_) && is_4_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == XX_X) && is_3_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == X_XX) && is_2_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == _XXX) && is_1_playable)
//                 (*connect3CountMax)++;
//             ///////////////////////////////////////////////////
//             else if (ID == OOOO)
//                 (*connect4CountMin)++;
//             else if ((ID == OOO_) && is_4_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == OO_O) && is_3_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == O_OO) && is_2_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == _OOO) && is_1_playable)
//                 (*connect3CountMin)++;
//         }
//     }

//     // scanning from up right to down left

//     for (int rowCursor = ROWS - 4 ; rowCursor >= 0; rowCursor--){
//         for ( int windows = 0 ; windows <= (ROWS - 4 - rowCursor) ; windows++){
//             int ID = 0;
//             for (int i = 0; i < 4; i++) {
//                 ID = 3 * ID + getEquivID(board[COLS * (rowCursor + windows + i) + (i + windows)]); // awel element howa bedayet el diagonal mn up left
//             }       

//             bool is_1_playable = (rowCursor + windows + 0) == nextFreeRow[windows + 0];
//             bool is_2_playable = (rowCursor + windows + 1) == nextFreeRow[windows + 1];
//             bool is_3_playable = (rowCursor + windows + 2) == nextFreeRow[windows + 2];
//             bool is_4_playable = (rowCursor + windows + 3) == nextFreeRow[windows + 3];

//             if (ID == XXXX)
//                 (*connect4CountMax)++;
//             else if ((ID == XXX_) && is_4_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == XX_X) && is_3_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == X_XX) && is_2_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == _XXX) && is_1_playable)
//                 (*connect3CountMax)++;
//             ///////////////////////////////////////////////////
//             else if (ID == OOOO)
//                 (*connect4CountMin)++;
//             else if ((ID == OOO_) && is_4_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == OO_O) && is_3_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == O_OO) && is_2_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == _OOO) && is_1_playable)
//                 (*connect3CountMin)++;  
//         }
//     }

//     // scanning from down left to up right
//     for (int rowCursor = 3 ; rowCursor <= ROWS - 1; rowCursor++){
//         for (int windows = 0 ; windows < rowCursor - 2 ; windows++){
//             int ID = 0;
//             for (int i = 0; i < 4; i++) {
//                 ID = 3 * ID + getEquivID(board[COLS * (rowCursor - windows - i) + (COLS - 1 - i - windows)]); // awel element howa bedayet el diagonal mn down right
//             }   

//             bool is_1_playable = (rowCursor - windows - 0) == nextFreeRow[COLS - 1 - windows - 0];
//             bool is_2_playable = (rowCursor - windows - 1) == nextFreeRow[COLS - 1 - windows - 1];
//             bool is_3_playable = (rowCursor - windows - 2) == nextFreeRow[COLS - 1 - windows - 2];
//             bool is_4_playable = (rowCursor - windows - 3) == nextFreeRow[COLS - 1 - windows - 3];      

//             if (ID == XXXX)
//                 (*connect4CountMax)++;
//             else if ((ID == XXX_) && is_4_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == XX_X) && is_3_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == X_XX) && is_2_playable)
//                 (*connect3CountMax)++;
//             else if ((ID == _XXX) && is_1_playable)
//                 (*connect3CountMax)++;
//             ///////////////////////////////////////////////////
//             else if (ID == OOOO)
//                 (*connect4CountMin)++;
//             else if ((ID == OOO_) && is_4_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == OO_O) && is_3_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == O_OO) && is_2_playable)
//                 (*connect3CountMin)++;
//             else if ((ID == _OOO) && is_1_playable)
//                 (*connect3CountMin)++;
//         }
//     }
// }








    // char board[ROWS * COLS] = {0} ; 
    // int  nextFreeRow[COLS] = {0} ;       // to keep track of the top row of each column in the board

    // initializeTopRow(nextFreeRow, COLS);
    // initializeBoard(board, ROWS, COLS);

    // // fill out the connect board with a full finshed game state
    // board[0]= 'o';
    // board[1]= 'o';
    // board[2]= EMPTY;
    // board[3]= EMPTY;
    // //
    // board[4]= 'x';
    // board[5]= 'o';
    // board[6]= 'x';
    // board[7]= 'x';
    // //
    // board[8]= 'x';
    // board[9]= 'x';
    // board[10]= 'x';
    // board[11]= 'o';
    // //
    // board[12]= 'x';
    // board[13]= 'x';
    // board[14]= 'x';     
    // board[15]= 'x';

    // printBoard(board , ROWS , COLS) ;   
    // printf("\n\n");

    // nextFreeRow[0] = -1 ; 
    // nextFreeRow[1] = -1;
    // nextFreeRow[2] = 0;
    // nextFreeRow[3] = 0 ;

    // printTopRow(nextFreeRow, COLS);
    // printf("\n\n");

    // next move (rowCursor - windows)

    // scanning 

    
    // int connect4CountMax = 0 ; 
    // int connect3CountMax = 0 ; 

    // int connect4CountMin = 0 ; 
    // int connect3CountMin = 0 ; 

    // scanHorizontal(board, nextFreeRow, &connect4CountMax, &connect3CountMax, &connect4CountMin, &connect3CountMin);
    // scanVertical(board, nextFreeRow, &connect4CountMax, &connect3CountMax, &connect4CountMin, &connect3CountMin);
    // scanDiagonal(board, nextFreeRow, &connect4CountMax, &connect3CountMax, &connect4CountMin, &connect3CountMin);
    

    // printf("connect4CountMax: %d\n", connect4CountMax);
    // printf("connect3CountMax: %d\n", connect3CountMax);
    // printf("connect4CountMin: %d\n", connect4CountMin);
    // printf("connect3CountMin: %d\n", connect3CountMin);