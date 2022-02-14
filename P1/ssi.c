//P1 Submission
//David Dennis V00914300
//February 11th 2022


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>



//linked list node setup
typedef struct bg_process{
	pid_t pid;
	char command[1024];
	struct bg_process* next;
}bg_process;

bg_process * add_to_list(pid_t pid, char ** commands, bg_process *head);

int
main()
{
	//setup intial variable space
	char path[512];
	char prompt[1024];
	char* usr = getlogin();
	char hostname[128];

	//setup root for process list
	bg_process *head = NULL;

	//change dir to home
	chdir(getenv("HOME"));

	//Start looping
	int bailout = 0;
	while (!bailout) {
		
		//get stuff for prompt
		getcwd(path, 512);
		usr = getlogin();
		gethostname(hostname, 128);
		sprintf(prompt, "%s@%s: %s > ", usr, hostname, path);
		
		//get input, using prompt
		char* reply = readline(prompt);

		//check for just an enter press
		if(!strcmp(reply, "")){
			//do nothing
			;
		//check for exit
		} else if (!strcmp(reply, "exit")) {
			bailout = 1;
		} else {
			//Separate inputs
			char* argv[80];
			argv[0] = strtok(reply, " \n");
			int i = 0;
			while(argv[i] != NULL){
				argv[i+1] = strtok(NULL, " \n");
				i++;	
			}

			//DO COMMANDS
			
			//directory change
			if(!strcmp(argv[0], "cd")){
				if(argv[2] != NULL){
					printf("\nUsage: cd <path>\n\n");
				}else if(argv[1] == NULL || strcmp(argv[1], "~") == 0){
					//change dir to home
					chdir(getenv("HOME"));
				}else{
					//absolute path
					if(argv[1][0] == '/'){
						chdir(argv[1]);
					//relative path
					}else{
						char buff[1024];
						sprintf(buff, "%s/%s", path, argv[1]);
						chdir(buff);
					}
				}
				
			//background process
			}else if(!strcmp(argv[0], "bg")){
				//adjust argv to be char ** then argv++ to get to argv[1]
				char **newarg = argv+1;
				//programs in directory
				if(**newarg == '.' || **newarg == '/'){
					pid_t pid = fork();
					if(pid==0){
						int status = execv(*newarg, newarg);
						if(status == -1) printf("file %s not found\n", *newarg);
						exit(0);
					}else{
						head = add_to_list(pid, newarg, head);
					}
				//basic tools in bin (ls, sleep, etc.)
				}else{
					pid_t pid = fork();
					if(pid==0){
						char arg1[128];
						sprintf(arg1, "/bin/%s", *newarg);
						int status = execvp(arg1, newarg);
						if(status == -1) printf("command %s not found\n", *newarg);
						exit(0);
					}else{
						head = add_to_list(pid, newarg, head);
					}
				}

			//check background list
			}else if(!strcmp(argv[0], "bglist")){
				bg_process* temp = head;
				int counter = 0;
				while(temp != NULL){
					printf("%d:%s\n", temp->pid, temp->command);
					counter++;
					temp = temp->next;
				}
				printf("Total Background jobs: %d\n", counter);

			//direct process
			}else{
				//programs in directorys
				if(argv[0][0] == '.' || argv[0][0] == '/'){
					pid_t pid = fork();
					if(pid==0){
						int status = execv(argv[0], argv);
						if(status == -1) printf("file %s not found\n", argv[0]);
						exit(0);
					}else{
						waitpid(pid, NULL, 0);
					}
				//basic tools in bin (ls, sleep, etc.)
				}else{
					pid_t pid = fork();
					if(pid==0){
						char arg1[128];
						sprintf(arg1, "/bin/%s", argv[0]);
						int status = execvp(arg1, argv);
						if(status == -1) printf("command %s not found\n", argv[0]);
						exit(0);
					}else{
						waitpid(pid, NULL, 0);
					}
				}
			}
		}

		//after dealing with input, check on BG processes
		if(head != NULL){
			pid_t terminate = waitpid(0, NULL, WNOHANG);
			while(terminate > 0){
				if(head->pid == terminate){
					printf("[%d]: %s has terminated\n", head->pid, head->command);
					bg_process *end = head;
					free(end);
					head = head->next;
					
				}else{
					bg_process *curr = head->next;
					while(curr->next->pid != terminate){
						curr = curr->next;
					}
					printf("[%d]: %s has terminated\n", curr->next->pid, curr->next->command);
					bg_process *end = curr->next;
					free(end);
					curr = curr->next->next;
				}
				terminate = waitpid(0, NULL, WNOHANG);
			}
		}
		free(reply);
	}
	
	//Exit code
	
	//kill all outstanding processes
	bg_process *curr = head;
	while(curr != NULL){
		kill(curr->pid, SIGKILL);
		curr = curr->next;
	}
	
	//free linked list
	while(head != NULL){
		bg_process *temp = head;
		head = head->next;
		free(temp);
	}
}

//takes the information from a process and adds it to the end of the list of background processes
bg_process * add_to_list(pid_t pid, char ** commands, bg_process *head){

	bg_process *temp = (bg_process *)malloc(sizeof(bg_process));
	assert(temp);

	//detokenize input to put into bg_process node as a single string
	char comstring[1024] = "";
	strcat(comstring, *commands);
	commands++;
	while(*commands != NULL){
		strcat(comstring, " ");
		strcat(comstring, *commands);
		commands++;
	}

	temp->pid = pid;
	strcpy(temp->command, comstring);
	temp->next = NULL;

	if(head==NULL){
		head = temp;
	}else{
		bg_process *curr = head;
		while(curr->next != NULL){
			curr = curr->next;
		}
		curr->next = temp;
	}
	return head;	
}
