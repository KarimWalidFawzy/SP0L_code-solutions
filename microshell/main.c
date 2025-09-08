/*
 * Micro Shell with I/O Redirection
 *
 * This program extends the shell command line parser to support I/O redirection.
 * It handles the following operators:
 * - < : Redirects the command's stdin to read from the specified file.
 * - > : Redirects the command's stdout to write to the specified file.
 * - 2> : Redirects the command's stderr to write to the specified file.
 *
 * Multiple redirections can be used in the same command line. If an error
 * occurs during redirection (e.g., file cannot be opened), the command is
 * not executed.
 *
 * This version uses standard Unix system calls like dup2, open, fork,
 * and execvp to ensure proper shell behavior and error handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h> // Include errno for strerror()

extern char** environ;

// A helper function to remove consecutive spaces from a string.
void rmcsp(char* str) {
    int len = strlen(str);
    int j = 0;
    bool last_char_ns = true;
    for (int i = 0; i < len; i++) {
        if (str[i] != ' ' || last_char_ns) {
            str[j] = str[i];
            j++;
        }
        if (str[i] == ' ') {
            last_char_ns = false;
        } else {
            last_char_ns = true;
        }
    }
    str[j] = 0;
}

// A helper function to clear the input buffer.
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int microshell_main(int argc, char* argv[]) {
    char buff[1 << 10];
    int last_command_status = 0; // Initialize status to success

    while (1) {
        printf(">");
        fflush(stdout);
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

        // --- Handle built-in variable assignment first ---
        char *equal_sign = strchr(buff, '=');
        if (equal_sign && strncmp(buff, "export", 6) != 0) {
            *equal_sign = '\0';
            char *var_name = buff;
            char *var_value = equal_sign + 1;
            rmcsp(var_name);
            rmcsp(var_value);
            if (setenv(var_name, var_value, 1) != 0) {
                perror("setenv failed");
                last_command_status = 1;
            } else {
                last_command_status = 0;
            }
            continue;
        }

        // --- Replace $variables after assignment check ---
        char processed_buff[1 << 10];
        int i = 0, j = 0;
        while (buff[i]) {
            if (buff[i] == '$') {
                i++;
                char var_name[256] = {0};
                int k = 0;
                while (buff[i] && buff[i] != ' ' && buff[i] != '\t' && buff[i] != '>' && buff[i] != '<') {
                    var_name[k++] = buff[i++];
                }
                char* var_value = getenv(var_name);
                if (var_value) {
                    strcpy(&processed_buff[j], var_value);
                    j += strlen(var_value);
                }
            } else {
                processed_buff[j++] = buff[i++];
            }
        }
        processed_buff[j] = '\0';
        strcpy(buff, processed_buff);
        rmcsp(buff);

        // --- I/O Redirection Parsing ---
        char* redirect_in_file = NULL;
        char* redirect_out_file = NULL;
        char* redirect_err_file = NULL;
        bool in_before_out[2] = {false,false}; // To track the order of < and >
        char* args[256];
        int arg_count = 0;
        
        char temp_buff[1 << 10];
        strcpy(temp_buff, buff);
        
        char* token = strtok(temp_buff, " \t");
        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                token = strtok(NULL, " \t");
                if (token) redirect_in_file = strdup(token);
				in_before_out[0] = true;
            } else if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " \t");
                if (token) redirect_out_file = strdup(token);
				if(in_before_out[0]) 
				{
					in_before_out[1] = true;
				}
            } else if (strcmp(token, "2>") == 0) {
                token = strtok(NULL, " \t");
                if (token) redirect_err_file = strdup(token);
            } else {
                args[arg_count++] = strdup(token);
            }
            token = strtok(NULL, " \t");
        }
        args[arg_count] = NULL;
        bool iob= in_before_out[0] && in_before_out[1];
        if (arg_count == 0) {
            for (int k = 0; k < arg_count; k++) {
                free(args[k]);
            }
            if (redirect_in_file) free(redirect_in_file);
            if (redirect_out_file) free(redirect_out_file);
            if (redirect_err_file) free(redirect_err_file);
            continue;
        }

        // --- Built-in Command Execution (in the main shell process) ---
        if (strcmp(args[0], "exit") == 0) {
            printf("Good Bye\n");
            return last_command_status;
        } else if (strcmp(args[0], "cd") == 0) {
            char* path = args[1];
            if (!path) {
                path = getenv("HOME");
                if (!path) {
                    fprintf(stderr, "cd: HOME not set\n");
                    last_command_status = 1;
                    continue;
                }
            }
            if (chdir(path) != 0) {
                perror("cd");
                last_command_status = 1;
            } else {
                last_command_status = 0;
            }
        } else if (strcmp(args[0], "export") == 0) {
            if (arg_count < 2) {
                fprintf(stderr, "export: missing argument\n");
                last_command_status = 1;
            } else {
                char* equal_sign = strchr(args[1], '=');
                if (equal_sign) {
                    *equal_sign = '\0';
                    if (setenv(args[1], equal_sign + 1, 1) != 0) {
                        perror("export");
                        last_command_status = 1;
                    } else {
                        last_command_status = 0;
                    }
                } else {
                    fprintf(stderr, "export: invalid syntax\n");
                    last_command_status = 1;
                }
            }
        } else if (strcmp(args[0], "printenv") == 0) {
            if (arg_count == 1) {
                for (char **env = environ; *env != NULL; env++) {
                    puts(*env);
                }
            } else {
                char* var_value = getenv(args[1]);
                if (var_value) {
                    puts(var_value);
                } else {
                    fprintf(stderr, "printenv: %s: No such environment variable\n", args[1]);
                    last_command_status = 1;
                }
            }
        } else {
            // --- External Command Execution (using fork and exec) ---
            
            // Save original file descriptors
            int original_stdin = dup(STDIN_FILENO);
            int original_stdout = dup(STDOUT_FILENO);
            int original_stderr = dup(STDERR_FILENO);
            
            bool redirection_failed = false;

            // Handle stderr redirection first to ensure error messages go to the right place
            if (redirect_err_file && !iob) {
                int fd_err = open(redirect_err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_err == -1) {
                    fprintf(stderr, "cannot access %s: %s\n", redirect_err_file, strerror(errno));
                    redirection_failed = true;
                } else {
                    dup2(fd_err, STDERR_FILENO);
                    close(fd_err);
                }
            }

            // Handle input redirection
            if (!redirection_failed && redirect_in_file || iob) {
                int fd_in = open(redirect_in_file, O_RDONLY);
                if (fd_in == -1 || iob) {
                    fprintf(stderr, "cannot access %s: %s\n", redirect_in_file, strerror(errno));
                    redirection_failed = true;
                } else {
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }
            }
            
            // Handle output redirection
            if (!redirection_failed && redirect_out_file) {
                int fd_out = open(redirect_out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out == -1) {
                    fprintf(stderr, "%s: %s\n", redirect_out_file, strerror(errno));
                    redirection_failed = true;
                } else {
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }
            }

            if (!redirection_failed) {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork failed");
                    last_command_status = 1;
                } else if (pid == 0) { // Child process
                    execvp(args[0], args);
                    perror(args[0]);
                    exit(127);
                } else { // Parent process
                    int status;
                    if (waitpid(pid, &status, 0) == -1) {
                        perror("waitpid");
                        last_command_status = 1;
                    } else {
                        if (WIFEXITED(status)) {
                            last_command_status = WEXITSTATUS(status);
                        } else {
                            last_command_status = 1;
                        }
                    }
                }
            } else {
                last_command_status = 1;
            }

            // Restore original file descriptors in the parent shell
            dup2(original_stdin, STDIN_FILENO);
            dup2(original_stdout, STDOUT_FILENO);
            dup2(original_stderr, STDERR_FILENO);
            close(original_stdin);
            close(original_stdout);
            close(original_stderr);
        }
        
        // Free allocated memory for tokens and file paths
        for (int k = 0; k < arg_count; k++) {
            free(args[k]);
        }
        if (redirect_in_file) free(redirect_in_file);
        if (redirect_out_file) free(redirect_out_file);
        if (redirect_err_file) free(redirect_err_file);
    }
    return last_command_status;
}
int main(int argc, char* argv[]) {
	return microshell_main(argc, argv);
}