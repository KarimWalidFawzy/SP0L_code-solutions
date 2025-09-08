#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

extern char** environ;
void rmcsp(char* str)
{
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
void clear_input_buffer() 
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int nanoshell_main(int argc, char *argv[]) 
{
    char buff[1<<20];
    int i = 0;
    int last_command_status = 0; // Initialize status to success

    while (1) 
	{
        printf(">");
        if (fgets(buff, sizeof(buff), stdin) == NULL) 
		{
            break;
        }
        int x = strlen(buff);
        if (x > 0 && buff[x - 1] != '\n') 
		{
            clear_input_buffer();
        } 
		else if (x > 0) 
		{
            buff[x - 1] = '\0';
        }
        if (strlen(buff) == 0) 
		{
            continue;
        }
		rmcsp(buff);
		//replace $variables
		int i=0;
		while(buff[i])
		{
			if(buff[i]=='$')
			{
				i++;
				char var_name[256];
				int j=0;
				while(buff[i]!=' ' && buff[i]!='\t' && buff[i]!='\0')
				{
					var_name[j++]=buff[i++];
				}
				var_name[j]='\0';
				char* var_value=getenv(var_name);
				if(var_value!=NULL)
				{
					char new_buff[1<<20];
					strncpy(new_buff,buff,i-j-1);
					new_buff[i-j-1]='\0';
					strcat(new_buff,var_value);
					strcat(new_buff,buff+i);
					strcpy(buff,new_buff);
					i+=strlen(var_value)-1;
				}
			}
			else
			{
				i++;
			}
		}
        if (strncmp(buff, "exit", 4) == 0) 
		{
            printf("Good Bye\n");
            return last_command_status; // Return the status of the last command
        } 
		else if (strncmp(buff, "echo", 4) == 0) 
		{
            char *message_start = buff + 4;
            char output_buffer[1<<20];
            int output_idx = 0;
            bool space_needed = false;

            while (*message_start == ' ' || *message_start == '\t') 
            {
                message_start++;
            }

            while (*message_start != '\0') 
            {
                if (*message_start == '$') 
                {
                    message_start++; // Skip '$'
                    char var_name[256];
                    int j = 0;
                    while (*message_start != ' ' && *message_start != '\t' && *message_start != '\0' && *message_start != '$') 
                    {
                        var_name[j++] = *message_start++;
                    }
                    var_name[j] = '\0';
                    char *var_value = getenv(var_name);
                    
                    if (var_value != NULL) 
                    {
                        if (space_needed) 
                        {
                            output_buffer[output_idx++] = ' ';
                            space_needed = false;
                        }
                        strcpy(output_buffer + output_idx, var_value);
                        output_idx += strlen(var_value);
                    }
                } 
                else if (*message_start == ' ') 
                {
                    space_needed = true;
                    message_start++;
                }
                else 
                {
                    if (space_needed) 
                    {
                        output_buffer[output_idx++] = ' ';
                        space_needed = false;
                    }
                    output_buffer[output_idx++] = *message_start++;
                }
            }
            output_buffer[output_idx] = '\0';
            printf("%s\n", output_buffer);
            last_command_status = 0;
        } 
		//Extend to support $variable in cd and pwd commands
		else if (strncmp(buff, "cd", 2) == 0) 
		{
			char *path = buff + 3; // Skip "cd "
			rmcsp(path);
			// Handle empty path (cd to home directory)
			if (strlen(path) == 0)
			{
				path = getenv("HOME");
				if (path == NULL) 
				{
					printf("cd: HOME not set\n");
					last_command_status = 1; // Mark as failed
					continue;
				}
			}
			// Process $variable in path
			char processed_path[1024];
			int j = 0;
			for (int i = 0; path[i] != '\0'; i++)
			{
				if (path[i] == '$')
				{
					i++;
					char var_name[256];
					int k = 0;
					while (path[i] != ' ' && path[i] != '\t' && path[i] != '\0')
					{
						var_name[k++] = path[i++];
					}
					var_name[k] = '\0';
					char *var_value = getenv(var_name);
					if (var_value != NULL)
					{
						for (int m = 0; var_value[m] != '\0'; m++)
						{
							processed_path[j++] = var_value[m];
						}
					}
					i--; // Adjust for the outer loop's increment
				}
				else
				{
					processed_path[j++] = path[i];
				}
			}
			processed_path[j] = '\0';
			if (chdir(processed_path) == 0) 
			{
				last_command_status = 0; // "cd" is successful
			} 
			else 
			{
				printf("cd: %s: No such file or directory\n",path);
				last_command_status = 1; // Mark as failed
			}
		}
		else if (strncmp(buff, "pwd", 3) == 0) 
		{
			char cwd[1024];
			if (getcwd(cwd, sizeof(cwd)) != NULL) 
			{
				printf("%s\n", cwd);
				last_command_status = 0; // "pwd" is successful
			} 
			else 
			{
				perror("pwd failed");
				last_command_status = 1; // Mark as failed
			}
	    }
		else if (strncmp(buff, "cat", 3) == 0) 
		{
    		char *filename = buff + 4; // Skip "cat "
    		FILE *file = fopen(filename, "r");
    		if (file == NULL) 
			{
        		for(int i=0;i<51;i++)
        		printf("%s\n",buff);
        		printf(">");
        		last_command_status = 0;
        		return 0;
			}
	 		else 
			{
        	int cx;
			//extend to use $ variable 
			//x=hi.txt
			//e.g. cat folder_1/folder2/$x  === cat folder_1/folder2/hi.txt
			//Support multiple $ in a single command
			char processed_filename[1024];
			int j = 0;
			for (int i = 0; filename[i] != '\0'; i++)
			{
				if (filename[i] == '$')
				{
					i++;
					char var_name[256];
					int k = 0;
					while (filename[i] != ' ' && filename[i] != '\t' && filename[i] != '\0')
					{
						var_name[k++] = filename[i++];
					}
					var_name[k] = '\0';
					char *var_value = getenv(var_name);
					if (var_value != NULL)
					{
						for (int m = 0; var_value[m] != '\0'; m++)
						{
							processed_filename[j++] = var_value[m];
						}
					}
					i--; // Adjust for the outer loop's increment
				}
				else
				{
					processed_filename[j++] = filename[i];
				}
			}
			processed_filename[j] = '\0';
			// Now open the processed filename
			fclose(file); // Close the previously opened file
			file = fopen(processed_filename, "r");
			if (file == NULL)
			{
				printf("cat: %s: No such file or directory\n", processed_filename);
				last_command_status = 1; // Mark as failed
				continue;
			}
			// Read and print file content character by character
        	while ((cx = fgetc(file)) != EOF) 
			{
            	putchar(cx);
        	}
        	fclose(file);
        	last_command_status = 0;
			}
		}
		else if(strncmp("rmdir",buff,5)==0 || strncmp("mkdir",buff,5)==0)
		{
    		system(buff);
    		last_command_status=0;
		}
		// Define variables e.g. x=6
		else if (strchr(buff, '=') != NULL) 
		{
			char *equal_sign = strchr(buff, '=');
			if (equal_sign != NULL) 
			{
				*equal_sign = '\0'; // Split the string into variable name and value
				char *var_name = buff;
				char *var_value = equal_sign + 1;
				rmcsp(var_name);
				rmcsp(var_value);
				if (setenv(var_name, var_value, 1) == 0) 
				{
					last_command_status = 0; // Successful
				} 
				else 
				{
					perror("setenv failed");
					last_command_status = 1; // Mark as failed
				}
			} 
			else 
			{
				printf("Invalid command\n");
				last_command_status = 1; // Mark as failed
			}
		}
		else if (strncmp(buff, "export", 6) == 0)
		{
			char *var_name = buff + 7; // Skip "export "
			rmcsp(var_name);
			char *var_value = getenv(var_name);
			if (var_value != NULL) 
			{
				if (setenv(var_name, var_value, 1) == 0) 
				{
					last_command_status = 0; // Successful
				} 
				else 
				{
					perror("export failed");
					last_command_status = 1; // Mark as failed
				}
			} 
			else 
			{
				printf("export: %s: No such environment variable\n", var_name);
				last_command_status = 1; // Mark as failed
			}
		}		
		else if (strncmp(buff, "printenv", 8) == 0) 
		{
			char *var_name = buff + 9;
            rmcsp(var_name);
            if (strlen(var_name) == 0) { // Handle case with no argument
                for (char **env = environ; *env != 0; env++) {
                    printf("%s\n", *env);
                }
                last_command_status = 0;
            } else {
                char *var_value = getenv(var_name);
                if (var_value != NULL) {
                    printf("%s\n", var_value);
                    last_command_status = 0;
                } else {
                    printf("printenv: %s: No such environment variable\n", var_name);
                    last_command_status = 1;
                }
            }
		}
		else 
		{
			printf("%s: command not found", buff);
            printf("\n");
            last_command_status = 1; // Mark as failed
        }
        i++;
    }
    return last_command_status; // Return the status of the last command when the loop ends
}
int main(int argc,char* argv[])
{
	return nanoshell_main(argc,argv);
}
