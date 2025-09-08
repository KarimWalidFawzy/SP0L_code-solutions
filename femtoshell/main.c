#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int femtoshell_main(int argc, char *argv[]) 
{
    char buff[1<<20];
    int i = 0;
    int last_command_status = 0; // Initialize status to success

    while (1) {
        printf(">");
        if (fgets(buff, sizeof(buff), stdin) == NULL) {
            break;
        }

        int x = strlen(buff);
        if (x > 0 && buff[x - 1] != '\n') {
            clear_input_buffer();
        } else if (x > 0) {
            buff[x - 1] = '\0';
        }

        if (strlen(buff) == 0) {
            continue;
        }

        if (strncmp(buff, "exit", 4) == 0) {
            printf("Good Bye\n");
            return last_command_status; // Return the status of the last command
        } else if (strncmp(buff, "echo", 4) == 0) {
            if (strlen(buff) >= 5) {
                printf("%s\n", (buff + 5));
                last_command_status = 0; // "echo" is a successful command
            } else {
                printf("Invalid command\n");
                last_command_status = 1; // Mark as failed
            }
        } else {
            printf("Invalid command\n");
            last_command_status = 1; // Mark as failed
        }
        i++;
    }
    return last_command_status; // Return the status of the last command when the loop ends
}
int main(int argc, char *argv[]) {
    return femtoshell_main(argc, argv);
}