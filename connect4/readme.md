## To run the game

``` bash

bash runGame.sh 8 true

```

- where the first value is the depth and the second is whether to use alpha beta pruning or no

## To run duel mode use 

``` bash 
bash OneVsOne.sh 8 4
```

## To print the tree use

``` bash 
gcc -std=c11 -O3 connect4_engine_forTree_Printing.c -o connect4_engine
CONNECT4_TRACE_REPLY=3 CONNECT4_TRACE_LAYERS=4 python3 connect4_gui.py --depth 4
```

## In a separate terminal run this 

``` python 
python3 view_game_tree.py GameTree.txt --reply 3
brave-browser "$PWD/GameTree_reply_3.html"

```
