// Include necessary header files
#include <linux/init.h> // defines initializations for macros that are called when module is loaded and unloaded.
#include <linux/kernel.h> // contains core kernel definitions
#include <linux/fs.h> // provide structures for creating a character device.
#include <linux/uaccess.h> // safe copying of data from kernel to user space.
#include <linux/miscdevice.h> //Alternative to full character device and doesn't need manual management of device numbers
#include <linux/random.h> // generate random numbers
#include <linux/module.h> // has definitions and function for kernel module development

// Function headers
static void reset_board(void);
static bool valid_move(int row, int col);
static void AI_move(void);
static ssize_t board_read(struct file *f, char __user *buf, size_t count, loff_t *pos);
static ssize_t board_write(struct file *f, const char __user *buf, size_t count, loff_t *pos);
static int __init ttc_init(void);
static void __exit ttc_exit(void);

// Included for good practice
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aminkeng Nkeng");
MODULE_DESCRIPTION("Created a kernel module where tic tac toe can be played");
MODULE_VERSION("1.0");

//data structures needed
typedef enum { 
    EMPTY = 0,
    X = 1,
    O = 2
}cell_state; // represents the possible state of each cell

typedef cell_state g_board[3][3]; // represents the tictactoe game board. 2d array
static g_board curr_board; //represents the current state of the board

// Specify and register the initialization and exit functions
module_init(ttc_init);
module_exit(ttc_exit);

//File oprations for the character device
static const struct file_operations ttc_fops = {
    .read = board_read,
    .write = board_write,
};

// misc device for the character device
static struct miscdevice ttc_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "tictactoe",
    .fops = &ttc_fops,
};

static void reset_board(void) { // reset game to initial state
    for (int row = 0; row < 3; row++) { // iterate through each row and column of the array
        for (int col = 0; col < 3; col++) {
            curr_board[row][col] = EMPTY; // set the value of each cell to EMPTY
        }
    }
}

static bool valid_move(int row, int col) { // checks whether move at row or colum is valid
    if (row < 0 || row > 2 || col < 0 || col > 2) {// check if range is valid
        return false;
    }
    return curr_board[row][col] == EMPTY; //checks if cell at row or column is empty
}

static bool make_move(int row, int col, cell_state player) { // make move on the board at specific location
    if (valid_move(row, col)) {
        curr_board[row][col] = player;// update the board with the symbol
        return true;
    }
    return false;
}

// checks all possible combinations for a winner. Returns X or O or EMPTY(if no one wins)
static cell_state check_winner(void) {
    for (int row = 0; row < 3; row++) {
        if (curr_board[row][0] == curr_board[row][1] && curr_board[row][1] == curr_board[row][2] && curr_board[row][0] != EMPTY) {
            cell_state winner = curr_board[row][0];
            printk(KERN_INFO "Row %d - Player %c wins!\n", row, (winner == X) ? 'X' : 'O');
            return winner;
        }
    }

    for (int col = 0; col < 3; col++) {
        if (curr_board[0][col] == curr_board[1][col] && curr_board[1][col] == curr_board[2][col] && curr_board[0][col] != EMPTY) {
            cell_state winner = curr_board[0][col];
            printk(KERN_INFO "Column %d - Player %c wins!\n", col, (winner == X) ? 'X' : 'O');
            return winner;
        }
    }

    if (curr_board[0][0] == curr_board[1][1] && curr_board[1][1] == curr_board[2][2] && curr_board[0][0] != EMPTY) {
        cell_state winner = curr_board[0][0];
        printk(KERN_INFO "Diagonal (top-left to bottom-right) - Player %c wins!\n", (winner == X) ? 'X' : 'O');
        return winner;
    }

    if (curr_board[0][2] == curr_board[1][1] && curr_board[1][1] == curr_board[2][0] && curr_board[0][2] != EMPTY) {
        cell_state winner = curr_board[0][2];
        printk(KERN_INFO "Diagonal (top-right to bottom-left) - Player %c wins!\n", (winner == X) ? 'X' : 'O');
        return winner;
    }

    printk(KERN_INFO "It's a tie! No winner yet.\n");
    return EMPTY;
}


static void AI_move(void) {// resposible for the AI making a move on the game board
    int row, col;
    do { //generates random row and column values
        get_random_bytes(&row, sizeof(row));
        get_random_bytes(&col, sizeof(col));
        row = row % 3; // ensure row of within 0-2 range
        col = col % 3; // ensure column is within 0-2 range
    } while (!valid_move(row, col));// used to find a vilid move

    make_move(row, col, O); // uses function to place its symbol
}

static ssize_t board_read(struct file *f, char __user *buf, size_t count, loff_t *pos) {
    // Function for the read operation of the character device
    // Reads the current state of the game board and returns it as a string to the user

    char gb_str[12]; // Buffer to store the game board as a string
    size_t len;

    // Convert the game board into a string representation
    //ternary operator is used to determine character representation based on the cell
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            gb_str[i * 4 + j] = (curr_board[i][j] == EMPTY) ? '-' : (curr_board[i][j] == X) ? 'X' : 'O';
        }
        gb_str[i * 4 + 3] = '\n'; // Add a newline character after each row
    }
    gb_str[11] = '$'; // Add a dollar sign as a prompt for the next u_comm

    len = min(count, sizeof(gb_str)); // Determine the actual length to copy

    if (*pos >= len) {
        return 0; // If the position is already at or beyond the length, return 0 to indicate end-of-file
    }

    if (copy_to_user(buf, gb_str, len)) { // using built in kernel function
        return -EFAULT; // Copy the board string to the user buffer, return -EFAULT on error
    }

    *pos += len; // Update the file position
    return len; // Return the number of bytes read
}

// Function for the write operation of the character device
// Processes user commands and performs the corresponding actions on the game board
static ssize_t board_write(struct file *f, const char __user *buf, size_t count, loff_t *pos) {
    char u_comm[10]; // Buffer to store the user u_comm
    int row, col;
    cell_state player = X; // Initialize the player as X (assuming X always goes first)
    cell_state winner;
    
    if (count > sizeof(u_comm) - 1) {
        return -EINVAL; // If the u_comm length exceeds the buffer size, return -EINVAL (invalid argument)
    }

    if (copy_from_user(u_comm, buf, count)) {
        return -EFAULT; // Copy the user u_comm from the user buffer, return -EFAULT on error
    }

    u_comm[count] = '\0'; // Null-terminate the u_comm string to determine its end

    if (strncmp(u_comm, "RESET", 5) == 0) {
        reset_board(); // If the u_comm is "RESET", reset the game board
        return count; // Return the number of bytes written which is stored in count
    }

    if (strncmp(u_comm, "BOARD", 5) == 0) {
        return count; // If the u_comm is "BOARD", return the number of bytes written
    }

    if (sscanf(u_comm, "TURN %d %d", &row, &col) == 2) {
        // If the u_comm is in the format "TURN row col", extract the row and col values
        if (valid_move(row, col)) {
            // Check if the move is valid
            make_move(row, col, player); // Make the move for the current player
            
            winner = check_winner(); // Call check_winner() to see if there's a winner
            
            if (winner == EMPTY) { // If there's no winner yet, let AI make its move
                AI_move(); // Allow the AI to make its move
                winner = check_winner(); // Call check_winner() again to see if AI's move resulted in a win or a tie
            }

            if (winner != EMPTY) { // If there is a winner or a tie
                reset_board(); // Reset the board for a new game
            }
        } else {
            printk(KERN_ERR "Invalid move. Please try again.\n"); // Print an error message for an invalid move
        }
        return count; // Return the number of bytes written (count)
    }

    return -EINVAL; // If none of the recognized commands are provided, return -EINVAL (invalid argument)
}


static int __init ttc_init(void) { // called during initialization of the module
    int ret;

    // Reset the game board to its initial state
    reset_board();

    // Register the misc device for the character device
    ret = misc_register(&ttc_misc_dev);
    if (ret) {
        pr_err("Failed to register the tictactoe misc device\n");
        return ret;
    }

    pr_info("Tic Tac Toe kernel module initialized\n");
    return 0;
}

static void __exit ttc_exit(void) { //called when module is being unloaded
    // Deregister the misc device for the character device
    misc_deregister(&ttc_misc_dev);

    pr_info("Tic Tac Toe kernel module exited\n");
}
