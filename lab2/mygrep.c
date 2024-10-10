#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     

void print_highlighted(const char *line, const char *pattern) {
    const char *ptr = line; // current position in the string
    const char *match;      // to store the address of a match

    // while there are matches in the string
    while ((match = strstr(ptr, pattern)) != NULL) {
        // Print the part of the string before the match
        fwrite(ptr, 1, match - ptr, stdout);
        // Print the matched part with red highlighting
        printf("\033[1;31m%.*s\033[0m", (int)strlen(pattern), match);
        // Move the pointer to the end of the match to continue searching the rest of the string
        ptr = match + strlen(pattern);
    }
    // Print the remaining part of the string after the last match
    printf("%s", ptr);
}

void grep_file(FILE *file, const char *pattern) {
    char line[1024]; // Declare the line variable

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, pattern) != NULL) { // Check if the line contains the search pattern
            print_highlighted(line, pattern);
        }
    }
}

// Main function of the program
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s pattern [file]\n", argv[0]);
        return 1;
    }

    // Store the search pattern
    const char *pattern = argv[1];

    // If only the pattern is provided, read from standard input
    if (argc == 2) {
        grep_file(stdin, pattern); // Call the function to read from stdin
    } else {
        // If a file is specified, open it for reading
        FILE *file = fopen(argv[2], "r"); 
        if (!file) { 
            perror("fopen"); 
            return 1; 
        }
        grep_file(file, pattern);
        fclose(file); 
    }

    return 0; 
}
