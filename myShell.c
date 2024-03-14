#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<readline/readline.h>
#include<fcntl.h>
#include <dirent.h> 

void executeCommand();

char commands[100][100][100]; //max arguments is 100,each arguments can have length 100
int commandArgumentCount[100]; 
int commandCount; 

char last16[16][10000];
int last16Count = 0;


void handleCommand(char *input)
{
	
	commandArgumentCount[commandCount] = 0;

	char buffer[100];
	int bufferLength = 0;

	int len = strlen(input);
	for(int i = 0; i < len; i++)
	{
		if(input[i] == ' ' || input[i] == '\t')
		{
			if(bufferLength > 0)
			{
				buffer[bufferLength] = '\0';
				strcpy(commands[commandCount][commandArgumentCount[commandCount]++],buffer);

				bufferLength = 0;
			}
		}
		else
		{
			buffer[bufferLength++] = input[i];
		}
	}
	if(bufferLength > 0)
	{
		buffer[bufferLength] = '\0';
		strcpy(commands[commandCount][commandArgumentCount[commandCount]++],buffer);

		bufferLength = 0;
	}

	commandCount++;

}

void SplitInput(char input[1000])
{
	commandCount = 0;
	
	char buffer[10000];
	int bufferLength = 0;

	int len = strlen(input);
	for(int i = 0; i < len; i++)
	{
		if(input[i] == '|')
		{
			if(bufferLength > 0)
			{
				buffer[bufferLength] = '\0';
				handleCommand(buffer);

				bufferLength = 0;
			}
		}
		else
		{
			buffer[bufferLength++] = input[i];
		}
	}
	if(bufferLength > 0)
	{
		buffer[bufferLength] = '\0';
		handleCommand(buffer);

		bufferLength = 0;
	}
	
}

void createProcess(int in, int out, int commandNum, int needPrint, int theLast)
{
	pid_t pid;
	pid = fork();

	if(pid == 0)
	{
		if(needPrint == 1)
		{
			printf("[Pid]: %d\n",getpid());
		}		
		
		if(in != 0)
		{
			dup2(in,0);
			close(in);
		}
		if(out != 1)
		{
			dup2(out,1);
			close(out);
		}
		
		int outputSymbolPos = -1;
		int inputSymbolPos = -1;
		for(int i = 0; i < commandArgumentCount[commandNum]; i++) //how many arguments
		{
			if((strcmp(commands[commandNum][i],">") == 0))
			{
				outputSymbolPos = i;
			}
			if((strcmp(commands[commandNum][i],"<") == 0))
			{
				inputSymbolPos = i;
			}
		}

		int argumentBound = commandArgumentCount[commandNum]; 

		if(outputSymbolPos > 0)
		{
			if(outputSymbolPos < argumentBound)
			{
				argumentBound = outputSymbolPos;
			}
			
			char *fName = commands[commandNum][outputSymbolPos+1];
			int fd; 
			if((fd = creat(fName,0644)) == -1)
			{
				printf("error!\n");
			}
			dup2(fd,STDOUT_FILENO);
			close(fd);
		}

		if(inputSymbolPos > 0)
		{
			if(inputSymbolPos < argumentBound)
			{
				argumentBound = inputSymbolPos;
			}

			char *fName = commands[commandNum][inputSymbolPos+1];
			int fd; 
			fd=open(fName, O_RDONLY, 0);
			dup2(fd,STDIN_FILENO);
			close(fd);
		}

		char *myCommand[argumentBound+1];
		myCommand[argumentBound] = NULL;
		for(int i = 0; i < argumentBound; i++)
		{
			myCommand[i] = malloc(strlen(commands[commandNum][i])+1);
			strcpy(myCommand[i],commands[commandNum][i]);
		}

		//cd and exit in pipe is meanless
		
		if(strcmp(myCommand[0],"help") == 0)
		{
			printf("==========================================================\n");
			printf("my little shell\n");
			printf("Type program names and arguments, and hit enter.\n");
			printf("\n");
			printf("The following are built in:\n");
			printf("1: help: show all build-in function info\n");
			printf("2: cd: change directory\n");
			printf("3: echo: echo the strings to standard output\n");
			printf("4: record: show last-16 cmds you typed in\n");
			printf("5: replay: re-execute the cmd showed in record\n");
			printf("6: mypid: find and print process-ids\n");
			printf("7: exit: exits hell\n");
			printf("\n");
			printf("Use the \"man\" command for information on other programs.\n" );
		}	
		else if(strcmp(myCommand[0],"cd") == 0)
		{
			if(argumentBound == 1)
			{
				//do nothing
			}
			else if(argumentBound == 2)
			{
				int v = chdir(myCommand[1]);
			}
			else
			{
				printf("cd: too many arguments\n");
			}
		}
		if(strcmp(myCommand[0],"exit") == 0)
		{
			if(argumentBound == 1)
			{
				printf("my little shell: See you next time.\n");
				exit(0);
			}
			else
			{
				printf("exit: too many arguments\n");
			}
		}
		else if(strcmp(myCommand[0],"echo") == 0)
		{
			int changeLine = 1;
			int first = 1;
			if(argumentBound > 1)
			{
				if(strcmp(myCommand[1],"-n") == 0)
				{
					changeLine = 0;
					for(int i = 2; i < argumentBound; i++)
					{
						if(first == 1)
						{
							printf("%s",myCommand[i]);
							first = 0;
						}
						else
						{
							printf(" %s",myCommand[i]);
						}
					}
				}
				else
				{
					for(int i = 1; i < argumentBound; i++)
					{
						if(first == 1)
						{
							printf("%s",myCommand[i]);
							first = 0;
						}
						else
						{
							printf(" %s",myCommand[i]);
						}
					}
				}
			}

			if(changeLine == 1)
			{
				printf("\n");
			}
			
			
		}
		else if(strcmp(myCommand[0],"record") == 0)
		{
			printf("history cmd:\n");
			for(int i = 0; i < last16Count; i++)
			{
				printf("%2d: %s\n", i+1,last16[i]);
			}
		}
		else if(strcmp(myCommand[0],"replay") == 0)
		{
			
 			int canExecute = 1;

			if(argumentBound != 2)
			{
				canExecute = 0;
			}

			int val = atoi(myCommand[1]);

			if(!(val >= 1 && val <= 16))
			{
				canExecute = 0;
			}

			
			if(canExecute == 1 && val <= last16Count)
			{
				SplitInput(last16[val-1]); //split the input string
				executeCommand();
			}
			else
			{
				printf("replay: wrong args\n");
			}
			
		}
		else if(strcmp(myCommand[0],"mypid") == 0)
		{
			pid_t myPid = getpid();
			if(argumentBound < 2)
			{
				printf("mypid: input error\n");
			}
			else if(argumentBound == 2)
			{
				if(strcmp(myCommand[1],"-i") == 0)
				{
					printf("%d\n",myPid);
				}
				else
				{
					printf("mypid: input error\n");
				}
			}
			else if(argumentBound == 3)
			{

				if(strcmp(myCommand[1],"-p") == 0)
				{
					char fileName[100] = "/proc/";
					strcat(fileName,myCommand[2]);
					strcat(fileName,"/status");
					

					FILE *fptr = fopen(fileName,"r");
					if(fptr)
					{
						char line[1024];

						while (fgets(line, sizeof(line), fptr)) 
						{
							if(line[0] == 'P' && line[1] == 'P' && line[2] == 'i' && line[3] == 'd')
							{
								for(int i = 4; i < strlen(line); i++)
								{
									if(line[i] >= '0' && line[i] <= '9')
									{
										printf("%c",line[i]);
									}
								}
								printf("\n");
								break;
							}
						}
					}
					else
					{
						printf("mypid: process id not exist\n");
					}
					fclose(fptr);
					
				}
				else if(strcmp(myCommand[1],"-c") == 0)
				{
					
					char fileName[100] = "/proc/";
					strcat(fileName,myCommand[2]);
					strcat(fileName,"/task/");
					strcat(fileName,myCommand[2]);
					strcat(fileName,"/children");

					FILE *fptr = fopen(fileName,"r");
					if(fptr)
					{
						char line[1024];

						if (fgets(line, sizeof(line), fptr)) 
						{
							int len = strlen(line);	
							char buffer[10];	
							int bufferLength = 0;
							for(int i = 0; i < len; i++)
							{
								if(line[i] >= '0' && line[i] <= '9')
								{
									buffer[bufferLength++] = line[i];
								}
								else
								{
									if(bufferLength > 0)
									{
										buffer[bufferLength] = '\0';
										printf("%s\n",buffer);
										bufferLength = 0;
									}
								}
							}
							if(bufferLength > 0)
							{
								buffer[bufferLength] = '\0';
								printf("%s\n",buffer);
								bufferLength = 0;
							}

						}
					}
					else
					{
						printf("mypid: process id not exist\n");
					}
					fclose(fptr);
					
					
				}
				else
				{
					printf("mypid: input error\n");
				}
			}
			else
			{
				printf("mypid: input error\n");
			}
			
			
		}
		else 
		{
			execvp(myCommand[0],myCommand);
		}

		
		exit(0);
	}
	else
	{
		if(theLast == 1)
		{	
			int status;
			waitpid(pid, &status, 0); 
		}
	}

}

void executeCommand()
{
	if(commandCount == 1) //single command
	{

		int backgroundExecution = 0;
		if(strcmp(commands[0][commandArgumentCount[0]-1],"&") == 0)
		{
			backgroundExecution = 1;
			commandArgumentCount[0]--;
		}

		if(backgroundExecution == 1)
		{
			createProcess(0,1,0,1,0);
		}
		else
		{
			if(strcmp(commands[0][0],"cd") == 0)
			{
				if(commandArgumentCount[0] == 1)
				{
					//do nothing
				}
				else if(commandArgumentCount[0] == 2)
				{
					int v = chdir(commands[0][1]);
				}
				else
				{
					printf("cd: too many arguments\n");
				}
			}
			if(strcmp(commands[0][0],"exit") == 0)
			{
				if(commandArgumentCount[0] == 1)
				{
					printf("my little shell: See you next time.\n");
					exit(0);
				}
				else
				{
					printf("exit: too many arguments\n");
				}
			}
			else
			{
				createProcess(0,1,0,0,1);
			}
		}

		
		
		

	}
	else //pipe
	{

		if(strcmp(commands[commandCount-1][commandArgumentCount[commandCount-1]-1], "&") == 0) //background
		{
			commandArgumentCount[commandCount-1]--;

			int i;
			pid_t pid;
			int in = 0;
			int fd[2];

			for(int i = 0; i < commandCount-1; i++)
			{
				int v = pipe(fd);
				createProcess(in,fd[1],i,0,0);
				close(fd[1]);
				in = fd[0];
			}

			createProcess(in,1,commandCount-1,1,0);
			
		}
		else
		{
			int i;
			pid_t pid;
			int in = 0;
			int fd[2];

			for(int i = 0; i < commandCount-1; i++)
			{
				int v = pipe(fd);
				createProcess(in,fd[1],i,0,0);
				close(fd[1]);
				in = fd[0];
			}

			createProcess(in,1,commandCount-1,0,1);

			
	
		}
		
		
	}

	

}


int main()
{
	for(int i = 0; i < 16; i++)
	{
		last16[i][0] = '\0';
	}

	printf("Welcome for using my little shell, type \"help\" for help\n");

	while(1)
	{
		char *input = readline(">>> $ "); //read command

		int haveContent = 0;

		for(int i = 0; i < strlen(input); i++)
		{
			if(!(input[i] == ' ' || input[i] == '\t'))
			{
				haveContent = 1;
				break;
			}
		}

		if(haveContent == 1)//if input is not empty
		{
			SplitInput(input); //split the input string
			
			int haveReplay = 0;
			if(commandCount == 1 && strcmp(commands[0][0],"replay") == 0)
			{
				haveReplay = 1;
			}

			if(haveReplay == 0)
			{

				char tempStr[100000];
				tempStr[0] = '\0';

				for(int i = 0; i < commandCount; i++)
				{
					if(i != 0)
					{
						strcat(tempStr," | ");
					}


					if(strcmp(commands[i][0],"replay") == 0)
					{
						if(commandArgumentCount[i] >= 2)
						{
							int val = atoi(commands[i][1]);
							if(val >= 1 && val <= 16)
							{
								strcat(tempStr, last16[val-1]);
							}
							else
							{
								for(int j = 0; j < commandArgumentCount[i]; j++)
								{
									if(j == 0)
									{
										strcat(tempStr, commands[i][j]);
									}
									else
									{	
										strcat(tempStr, " ");
										strcat(tempStr, commands[i][j]);
									}
								}
							}
						}
						else
						{
							for(int j = 0; j < commandArgumentCount[i]; j++)
							{
								if(j == 0)
								{
									strcat(tempStr, commands[i][j]);
								}
								else
								{	
									strcat(tempStr, " ");
									strcat(tempStr, commands[i][j]);
								}
							}
						}
					}
					else
					{
						for(int j = 0; j < commandArgumentCount[i]; j++)
						{
							if(j == 0)
							{
								strcat(tempStr, commands[i][j]);
							}
							else
							{	
								strcat(tempStr, " ");
								strcat(tempStr, commands[i][j]);
							}
						}
					}

					
				}

				if(last16Count < 16)
				{
					strcpy(last16[last16Count++],tempStr);
				}
				else
				{
					for(int i = 0; i < 15; i++)
					{
						strcpy(last16[i],last16[i+1]);
					}
					strcpy(last16[15],tempStr);
				}
			}

			

			

			executeCommand();
		}
	}
}
