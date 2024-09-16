#!/bin/bash 


file="$1"

if [ ! -f "$file" ]; then
    echo "File does not exist: $file"
    exit 1
fi  

# Extract metadata from the input PGN file
metadata=$(grep -E "^\[.*\]" "$file")

# Output the extracted metadata
echo "Metadata from PGN file:"
echo "$metadata"  

# Extract moves from the PGN file
file_moves=$(tail -n +11 "$file")

# Convert moves to UCI format using the Python script
uci_moves=$(python3 parse_moves.py "$file_moves") 

# Lets split the string into an array of moves
IFS=' ' read -r -a array_of_moves <<< "$uci_moves" 
count_moves=0 

# This function will initialize our board when we need it to be done.
initialize_board(){
    board=(
        "r n b q k b n r"
        "p p p p p p p p"
        ". . . . . . . ."
        ". . . . . . . ."
        ". . . . . . . ."
        ". . . . . . . ."
        "P P P P P P P P"
        "R N B Q K B N R"
    )
}  

# Lets make an associative array of the eaten soldiers
declare -A eaten_soldiers

# Lets make an array for the promoted pawns
declare -a promoted_pawns=()  

# This function prints and displays the current state of the chessboard
display_board() {
    echo "Move $count_moves/${#array_of_moves[@]}"
    echo "  a b c d e f g h"
    for ((i=0; i<8; i++)); do
        rows=$((8 - i))
        current_row="${board[i]}"
        pieces=($current_row)
        printf "%d" "$rows"
        for pi in "${pieces[@]}"; do
            printf "%2s" "$pi"
        done
        printf " %d\n" "$rows"
    done
    echo "  a b c d e f g h"
}  

# This function converts the coordinates of the move into an array index for the board
convert_to_board_index() {
    local all_letters="abcdefgh"
    local all_numbers="87654321"  
    local letter=$(echo "$1" | cut -c 1)
    local number=$(echo "$1" | cut -c 2)
    local row_index=$(expr index "$all_letters" "$letter")
    local col_index=$(expr index "$all_numbers" "$number")
    echo "$((col_index - 1)) $((row_index - 1))" 
} 

# This function handles the move.
make_a_move() {

    local current_move="$1"
    local move_from=${current_move:0:2}
    local move_to=${current_move:2:2}

     # Lets check if there is a need for promotion
    if [[ "$current_move" =~ ^[a-h][27][a-h][18][qrbn]$ ]]; then
        promotion=${current_move:4:1}
        promoted_pawns+=("$count_moves") 
        echo "moves count:" $count_moves
        echo "promoted_pawns_indexes:" $promoted_pawns
    fi

    # Lets convert the coordinates of the "from" and "to" into indexes
    local from_index=($(convert_to_board_index "$move_from"))
    local to_index=($(convert_to_board_index "$move_to"))

    # Lets take the soldier piece from the 'from' index and then remove it from there
    local piece=${board[from_index[0]]:from_index[1]*2:1}
    board[from_index[0]]="${board[from_index[0]]:0:from_index[1]*2}. ${board[from_index[0]]:from_index[1]*2+2}"

    # Lets add the eaten soldier into the array if its needed.
    local has_eaten=${board[to_index[0]]:to_index[1]*2:1}
    if [ "$has_eaten" != "." ]; then
        eaten_soldiers["$current_move"]="$has_eaten"
    fi

    # if there was a promotion, lets update the soldier to be the new one.
    if [ -n "$promotion" ]; then
        piece="$promotion"

    fi 

    # Lets put the soldier in the "to" index
    board[to_index[0]]="${board[to_index[0]]:0:to_index[1]*2}$piece ${board[to_index[0]]:to_index[1]*2+2}"

    # Lets also update the moves history array
    moves_history+=("$current_move")
}  

# This function handles the "next_move" option of the task.
next_move() {

    local show_board="${1:-true}"
    if (( count_moves < ${#array_of_moves[@]} )); then
        make_a_move "${array_of_moves[$count_moves]}"
        (( count_moves++ ))
    else
        echo "No more moves available."
        return  
    fi 

    if $show_board; then
        display_board
    fi
} 

# This function handles the cancelation of a move.
cancel_a_move() {

    if (( ${#moves_history[@]} > 0 )); then
        
        # Lets remove the last move from the history array
        local last_move="${moves_history[-1]}"
        moves_history=("${moves_history[@]:0:${#moves_history[@]}-1}")
        
        # Lets get the coordinates of the "from" and "to" of the last move
        local last_move_from=${last_move:0:2}
        local last_move_to=${last_move:2:2}

        # Lets convert them into indexes
        local from_index=($(convert_to_board_index "$last_move_from"))
        local to_index=($(convert_to_board_index "$last_move_to"))

        # Lets take the last move soldier piece info
        local piece=${board[to_index[0]]:to_index[1]*2:1} 
        local has_eaten="${eaten_soldiers[$last_move]}"

        # Lets move the soldier piece back to its last position.
        board[to_index[0]]="${board[to_index[0]]:0:to_index[1]*2}. ${board[to_index[0]]:to_index[1]*2+2}"
        board[from_index[0]]="${board[from_index[0]]:0:from_index[1]*2}$piece ${board[from_index[0]]:from_index[1]*2+2}"

         # Lets bring back the eaten soldier if its needed
        if [ -n "$has_eaten" ]; then
            board[to_index[0]]="${board[to_index[0]]:0:to_index[1]*2}$has_eaten${board[to_index[0]]:to_index[1]*2+1}"
            unset eaten_soldiers["$last_move"]
        fi

        # Lets check if the last move is in the array of promoted pawns, if it does we handle it.
        for p in "${!promoted_pawns[@]}"; do
            if [ "${promoted_pawns[$p]}" == "$count_moves" ]; then
                if [[ "$piece" =~ [A-Z] ]]; then
                    piece="P"
                else
                    piece="p"
                    echo
                fi
           
                board[from_index[0]]="${board[from_index[0]]:0:from_index[1]*2}$piece ${board[from_index[0]]:from_index[1]*2+2}"
                unset "promoted_pawns[$p]"  
                break
            fi
        done

    else
        echo "No moves to cancel."
    fi
} 

# This function handles the "previous move" option of the task.
previous_move() {
    if (( count_moves > 0 )); then
        (( count_moves-- ))
        cancel_a_move "${array_of_moves[$count_moves]}"
        display_board
        
    else
        display_board
    fi
} 

# This function handles the "go to start" option of the task.
go_to_start() {
    initialize_board
    count_moves=0
    moves_history=()
    display_board
}

# This function handles the "go to end" option of the task.
go_to_end() { 
    # This time we dont wanna display the board in "next_move" so we send argument of false
    while (( count_moves < ${#array_of_moves[@]} )); do
         next_move false 
    done
    display_board
    
}


main(){
    # Lets initialize the game
    echo 
    go_to_start

    # Lets start the loop of the game
    while true; do
        
        echo -n "Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit:"
        read -n 1 -s letter
        echo
        read -n 1 -s
        case "$letter" in
            "d")
                next_move
                ;;
            "a")
                previous_move
                ;;
            "w")
                go_to_start
                ;;
            "s")
                go_to_end
                ;;
            "q")
                echo "Exiting."
                echo "End of game."
                break
                ;;
            *)
                echo "Invalid key pressed: $letter"
                ;;
        esac
    done
}  

main




















