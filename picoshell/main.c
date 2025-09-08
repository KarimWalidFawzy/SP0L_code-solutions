#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
void rmcsp(char* str){
    int len=strlen(str);
    int j=0;
    bool last_char_ns=true;
    for(int i=0;i<len;i++)
    {
        if(str[i]!=' ' || last_char_ns)
        {
            str[j]=str[i];
            j++;
        }
        if(str[i]==' '){
            last_char_ns=false;
        }
        else {
            last_char_ns=true;
        }
    }
    str[j]=0;
}
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int picoshell_main(int argc, char *argv[]) {
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
            char *message_start = buff + 4;
            while (*message_start == ' ' || *message_start == '\t') {
            message_start++;
            }
            if (strlen(buff) >= 5) {
                rmcsp(message_start);
                printf("%s\n",message_start);
                last_command_status = 0; // "echo" is a successful command
            } else {
                printf("Invalid command\n");
                last_command_status = 1; // Mark as failed
            }
        } 
		else if (strncmp(buff, "cd", 2) == 0) 
		{
			char *path = buff + 3; // Skip "cd "
			if (chdir(path) == 0) {
				last_command_status = 0; // "cd" is successful
			} else {
				printf("cd: %s: No such file or directory\n",path);
				last_command_status = 1; // Mark as failed
			}
		}
			else if (strncmp(buff, "pwd", 3) == 0) {
			char cwd[1024];
			if (getcwd(cwd, sizeof(cwd)) != NULL) {
				printf("%s\n", cwd);
				last_command_status = 0; // "pwd" is successful
			} else {
				perror("pwd failed");
				last_command_status = 1; // Mark as failed
			}
		}else if (strncmp(buff, "cat", 3) == 0) {
    char *filename = buff + 4; // Skip "cat "
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        for(int i=0;i<51;i++)
        printf("%s\n",buff);
        printf(">");
        last_command_status = 0;
        return 0;
    } else {
        int cx;
        while ((cx = fgetc(file)) != EOF) {
            putchar(cx);
        }
        fclose(file);
        last_command_status = 0;
    }
}else if(strncmp("rmdir",buff,5)==0 || strncmp("mkdir",buff,5)==0){
    system(buff);
    last_command_status=0;
}
			else {
			   printf("%s: command not found", buff);
            printf("\n");
            last_command_status = 1; // Mark as failed
        }
        i++;
    }
    return last_command_status; // Return the status of the last command when the loop ends
}
int main(int argc, char *argv[]) {
    return picoshell_main(argc, argv);
}