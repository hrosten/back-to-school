#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////

#define EMPTY '.'
#define FILLED '#'
#define MAX_ROUNDS 100

// MAX_LINE_LEN gives some reasonable upper limit for the line length.
// 10240 characters seems plenty, but this can be increased to a lot bigger 
// value if needed. The upper limit is SSIZE_MAX.
// On a typical system, the value of SSIZE_MAX is the maximum value of signed 
// long, which is usually 2^63 (64-bit systems) or 2^31 (32-bit systems).
#define MAX_LINE_LEN 1024*10

// TEMPLINE is used as a temporary buffer for the line strings.
// Uninitialized globals are allocated outside of stack in the BSS segment. 
// Therefore, we will not overflow the stack even if the MAX_LINE_LEN is 
// increased to a value bigger than the stack size.
static char TEMPLINE[MAX_LINE_LEN];

////////////////////////////////////////////////////////////////////////////////

char* stripLeft(char* line) {
    // In-place strip whitespaces from the beginning of the line
    while((*line) == ' ') {
        line++;
    }
    return line;
}

char* stripRight(char* line) {
    // In-place strip whitespaces from the end of the line
    int len = strlen(line);
    if(len == 0) {
        return line;
    }
    char* pos = line + len - 1;
    while(pos >= line && (*pos) == ' ') {
        *pos = '\0';
        pos--;
    }
    return line;
}

char* strip(char* line) {
    // Strip whitespaces from both ends of the line.
    // Trimming is done in-place, meaning the pointer is simply forwarded 
    // past any whitespaces on the beginning
    // of the line and terminating character is wound backwards past 
    // trailing whitespaces.
    return stripRight(stripLeft(line));
}

////////////////////////////////////////////////////////////////////////////////

typedef struct StackEntry {
    // Pointer to next entry
    struct StackEntry* next;
    // Line string
    char* data;
    // Line string length
    int dataStrlen;
    // Line with whitespaces removed from both ends
    char* dataStripped;
    // Position of this entry from the bottom of the stack. On the first entry, 
    // (pos+1) equals number of entries pushed on the stack.
    uint8_t pos;
} StackEntry;

StackEntry* push(StackEntry* head, char* data) {
    // Push new entry on top of the stack
    StackEntry* new = calloc(1, sizeof(StackEntry));
    if (new == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    new->data = data;
    new->dataStrlen = strlen(data);
    // We use TEMPLINE as a temporary buffer for the stripped line
    memcpy(TEMPLINE, new->data, new->dataStrlen + 1);
    char* tmp = strip(TEMPLINE);
    new->dataStripped = strdup(tmp);
    new->next = head;
    if(head != NULL) {
        new->pos = head->pos + 1;
    }
    head = new;
    return head;
}

////////////////////////////////////////////////////////////////////////////////

int getFirstFilledIdx(char* line) {
    // Return the index of the first filled square on the given line 
    int len = strlen(line);
    for(int i=0; i < len; i++) {
        if(line[i] != ' ') {
            return i;
        }
    }
    return len;
}

int getLastFilledIdx(char* line) {
    // Return the index of the last filled square on the given line
    int len = strlen(line);
    for(int i=len-1; i >= 0; i--) {
        if(line[i] != ' ') {
            return i;
        }
    }
    return len;
}

int countFilled(char* line, int len) {
    // Count the number of filled squares on the given line
    int filled = 0;
    for(int i=0; i < len; i++) {
        if(line[i] != ' ') {
            filled++;
        }
    }
    return filled;
}

char* padTrimLine(char* line) {
    // Allocate and return a pointer to a new line. The returned string
    // will have at least 3 whitespaces in the beginning of the string
    // followed by the meaningful content of the old line with exactly 
    // 3 whitespaces at the end of the line.
    //
    // Why 3? 
    // We need at least 3 leading and trailing blanks so we can safely apply
    // the given rules on the line below.
    // Leading spaces are meaningful when determining if the pattern is
    // gliding or blinking, therefore, leading blanks are kept. 
    // Trailing blanks in excess of 3 can be removed since we know they will
    // not produce any new filled squares on the lines below the current line.
    int len = strlen(line);
    int firstFilledIdx = getFirstFilledIdx(line);
    int lastFilledIdx = getLastFilledIdx(line);
    int lfill = 3 - firstFilledIdx;
    int rfill = 3 - (len - lastFilledIdx -1);
    if(lfill < 0) 
        lfill = 0;
    int newlen = lfill + len + rfill + 1;
    char* newline = malloc(sizeof(char)*newlen);
    if(newline == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    memset(newline, ' ', newlen-1);
    newline[newlen-1] = '\0';
    memcpy(newline+lfill, line, lastFilledIdx + 1);
    return newline;
}

////////////////////////////////////////////////////////////////////////////////

typedef struct GameState {
    // Stack of lines: linesHead always points to the last added line
    StackEntry* linesHead;
} GameState;

GameState* newGameState(char* firstline) {
    // Initialize a new GameState with the given firstline
    GameState* game = calloc(1, sizeof(GameState));
    if (game == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    game->linesHead = push(game->linesHead, firstline);
    return game;
}

void deallocateGameState(GameState* this) {
    StackEntry* tmp;
    StackEntry* next = this->linesHead;
    while(next != NULL) {
        tmp = next->next;
        free(next->data);
        free(next->dataStripped);
        free(next);
        next = tmp;
    }
    free(this);
}

uint8_t linesFilled(GameState* this) {
    // How many lines have been filled so far?
    return (this->linesHead->pos + 1);
}

void fillNextLine(GameState* this) {
    // "The filling of each square is defined by the square above it and 4
    // squares next to it (2 squares on each sides).
    //
    // Rule #1, the square above is blank: If there are 2 or 3 filled squares
    // in total next to it (taking into account 4 squares, 2 on each sides)
    // it will be filled. If not, it will be left blank.
    //
    // Rule #2, the square above is filled: If there are 2 or 4 squares
    // filled in total next to it (taking into account 4 squares, 2 on each
    // sides) it will be filled. If not, it will be left blank.

    // Line above is the line last pushed on the stack
    char* lineAbove = this->linesHead->data;
    // Include terminating character, therefore +1
    int len = this->linesHead->dataStrlen + 1;
    // startIdx is the index of the first potentially filled square. 
    // Squares before it will be empty. 
    int startIdx = getFirstFilledIdx(lineAbove) - 1;
    // stopIdx is the index of the last potentially filled square. 
    // Squares after it will be empty. 
    int stopIdx = getLastFilledIdx(lineAbove) + 1;

    // Use global variable TEMPLINE as a buffer when filling the line.
    // Initialize with whitespaces.
    memset(TEMPLINE, ' ', len-1);
    TEMPLINE[len-1] = '\0';

    for(int i=startIdx; i < stopIdx + 1; i++) {
        // Pointer to block of 5 squares: 2 on each side of lineAbove[i]
        char* block = &(lineAbove[i-2]);
        int filled = countFilled(block, 5);
        if(lineAbove[i] == ' ') {
            // Rule #1
            if(filled == 2 || filled == 3) {
                TEMPLINE[i] = FILLED;
            }
        }
        else {
            // Rule #2
            // Note: we count the number of filled squares over the
            // full block of 5 squares, therefore checking for 3 or 5,
            // not 2 or 4.
            if(filled == 3 || filled == 5) {
                TEMPLINE[i] = FILLED;
            }
        }
    }
    // padTrimLine allocates new buffer
    char* newline = padTrimLine(TEMPLINE);
    //printf("[+] newline:%s\n", newline);
    this->linesHead = push(this->linesHead, newline);
}

bool printPattern(GameState* this) {
    // Prints the pattern name and returns true if the pattern can be
    // recognized based on the lines filled thus far. Otherwise, return false.
    char* lastline = this->linesHead->data;
    char* lastlineStripped = this->linesHead->dataStripped;

    // Loop through all but last added line comparing the last added
    // line with the previous lines.
    StackEntry* entry;
    for(entry = this->linesHead->next; entry != NULL; entry = entry->next) {
        // vanishing: there are no colored squares on a line
        if(lastlineStripped[0] == '\0') {
            printf("vanishing\n");
            return true;
        }
        // blinking: the pattern and location of colored squares is exactly 
        // the same as in some of the preceding lines
        if(strcmp(lastline, entry->data) == 0) {
            printf("blinking\n");
            return true;
        }
        // gliding: the pattern of colored squares is the same as in some of 
        // the preceding lines, but is located in different position
        if(strcmp(lastlineStripped, entry->dataStripped) == 0) {
            printf("gliding\n");
            return true;
        }
    }

    // other: None of the preceding types is detected when the last line was 
    // reached
    if(linesFilled(this) >= MAX_ROUNDS) {
        printf("other\n");
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void play(char* textfile) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE *fp = fopen(textfile,"r");

    if(!fp) {
        fprintf(stderr,"ERROR: failed to open file: \"%s\"\n", textfile);
        exit(1);
    }

    while((read = getline(&line, &len, fp)) != -1) {
        // Ignore blank lines
        if(read == 1) {
            continue;
        }
        if(read > MAX_LINE_LEN) {
            fprintf(stderr,
                "ERROR: file contains lines longer than %d characters\n", 
                MAX_LINE_LEN);
            exit(1);
        }
        // Remove newline from the end of the line
        if(line[read-1] == '\n') {
           line[read-1] = '\0';
           --read;
        }
        // Check that the line contains only expected characters.
        // Replace EMPTY markers from the input line with whitespaces.
        for(int i = 0; i < read; ++i) {
            if(!(line[i] == EMPTY || line[i] == FILLED)) {
                fprintf(stderr,
                    "ERROR: unexpected characters on a line: \"%c\"\n",line[i]);
                exit(1);
            }
            if(line[i] == EMPTY) {
                line[i] = ' ';
            }
        }
        char* trimmed = padTrimLine(line);
        //printf("[+] first  :%s\n", trimmed);

        GameState* game = newGameState(trimmed);
        while(linesFilled(game) < MAX_ROUNDS) {
            fillNextLine(game); 
            if(printPattern(game)) {
                break;
            }
        }
        deallocateGameState(game);
    }

    fclose(fp);
    free(line);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <textfile>\n", argv[0]);
        return 0;
    }

    char *textfile = argv[1];
    play(textfile);
}

////////////////////////////////////////////////////////////////////////////////
