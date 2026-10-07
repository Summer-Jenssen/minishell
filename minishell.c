#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t interrupted = 0;
int isChild = 0;

/**Returns 1 if it detected a ctrl c, otherwise checks errno to see if we found a problem :() */
int checkErrorNo(char* str){
    //check interrupted
    if(interrupted == 1){
        printf("\n");
        interrupted = 0; 
        errno = 0;
        if(isChild == 1){
            exit(EXIT_SUCCESS);
        } else {
        return 1;
        }
    }
    else if(errno != 0){
        perror(str);
        exit(EXIT_FAILURE);
    }
    return 0;
    //add sigint check here
}

/**SIGINT signal handler, sets our variable to 1 so checkErrorNo will pick it up later */
void handle_sigint(int sig) {
    interrupted = 1;
}

/**finds the number of digits in int a, used for text alignment later */
int get_num_digits(int a){
    int currCount = 0;
        while(a > 0){
            a = a/10;
            currCount++;
        }
    if(currCount == 0){
        currCount = 1;
    }
    return currCount;
}

/**finds the largest number of digits a value in array arr has, used for text alignment */
int largest_num_digits(int *arr, int numElems){
    int max = 0;
    for(int i = 0; i < numElems; i++){
        int currCount = get_num_digits(arr[i]);
        if (currCount > max){
            max = currCount;
        }
        
    }
    return max;
}

/**Used for qsort, compares two integers */
int cmpr_int(const void* a, const void* b){
    int* aa = (int*) a;
    int* bb = (int*) b;
    if ((*aa) > (*bb)){
        return 1;
    } else if ((*aa) == (*bb)){
        return 0;
    } else {
	return -1;
    }
}

/**Returns a 1 if str only contains numbers */
int onlyNums (char* str){
    int isnum = 0;
    for(int i = 0; str[i] != 0; i++){
        for (int j = 0; j<10; j++ ){
            if(str[i] == j+48){
                isnum =1;
            }
        } 
        if (isnum != 1){
            return 0;
        } else {
            isnum = 0;
        }   
    }
    return 1;
}

/**counts the number of dirs in the path proc */
int count_dirs(char* proc){
    DIR* dp = opendir(proc);
    if(!dp){
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent* dirp;
    char* name;
    char temp[256];
    
    strcpy(temp, proc);
    struct stat info;

    int count =0;
    
   
    while((dirp = readdir(dp)) != NULL){
        name = dirp->d_name;
        if(strcmp(name, ".") == 0|| strcmp(name, "..") == 0){
            continue;
        }
        strcat(temp, name);
        int a = stat(temp, &info);

        if(a == -1){
            perror("stat");
            exit(EXIT_FAILURE);
        }

        if(S_ISDIR(info.st_mode) && onlyNums(name)){
            count++;
        }

        strcpy(temp, proc);

    }
    if (errno != 0){
        perror("readdir");
        exit(EXIT_FAILURE);
    }
    
    int a = closedir(dp);
    if(a == -1){
        perror("closedir");
        exit(EXIT_FAILURE);
    }

    return count;
}


/**qsorts our array of pids */
void qsortPids(char* proc, int pids[], int numPids, int (*compar)(const void *, const void *)){
    DIR* dp = opendir(proc);
    if(!dp){
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent* dirp;
    char* name;
    char temp[256];
    
    strcpy(temp, proc);
    struct stat info;

    int count =0;
    
   
    while((dirp = readdir(dp)) != NULL){
        name = dirp->d_name;
        if(strcmp(name, ".") == 0|| strcmp(name, "..") == 0){
            continue;
        }
        strcat(temp, name);
        int a = stat(temp, &info);

        if(a == -1){
            perror("stat");
            exit(EXIT_FAILURE);
        }

        if(S_ISDIR(info.st_mode) && onlyNums(name)){
            pids[count] = atoi(name);
            count++;
        }

        strcpy(temp, proc);

    }
    if (errno != 0){
        perror("readdir");
        exit(EXIT_FAILURE);
    }
    
    int a = closedir(dp);
    if(a == -1){
        perror("closedir");
        exit(EXIT_FAILURE);
    }


    qsort(pids, numPids, sizeof(int), compar);

}


/**does the printing of the lp function, as well as finding directory owners and the commands that eecuted them */
void lp(char* proc, int isFirst, int arr[], int numPids){
    for(int i = 0; i < numPids; i++){
    // printf("Loop number: %d", i);
    char pid[12];
    char* user;
    char command[1024];
    char currPath[1024] = "";

    //get pid (as a string)
    
    sprintf(pid, "%d", arr[i]);
    
    
    //get user
    strcat(currPath, proc);
    strcat(currPath, pid);
    // printf("Curr Path: %s", currPath);
    struct stat info;

    int a = stat(currPath, &info);

    if(a == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    struct passwd *pw = getpwuid(info.st_uid);
    checkErrorNo("getpwuid");
    user = pw->pw_name;

    //get command -- HELP! How to make sure I have enough
    FILE *fptr;
    strcat(currPath, "/cmdline");

    fptr = fopen(currPath, "r");
    checkErrorNo("fopen");
    fgets(command, sizeof(command), fptr);
    checkErrorNo("fgets");
    fclose(fptr); 
    checkErrorNo("fclose");
    
    //alignment :(
    int max = largest_num_digits(arr, numPids);\
    int currNum = get_num_digits(arr[i]);
    int s = max-currNum;
    char space[10] = "";
    char* spp = " ";
    
    for(int i = 0; i < s; i++){
        strcat(space, spp);
    }
    
    //print the thing
    if (s != 0){
    printf("%s%s %s %s\n", space, pid, user, command);
    } else { //just in case... it works so I'm not taking it out. 
    printf("%s %s %s\n", pid, user, command);
    }
    
    }
}


/**Searches for all directories and files in char* directory, then prints them. */
void search_dirs(char* directory){
    DIR* dp = opendir(directory);
    if(!dp){
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent* dirp;
    char* name;
    char temp[256];
    
    strcpy(temp, directory);
    struct stat info;

    
   
    while((dirp = readdir(dp)) != NULL){
        name = dirp->d_name;
        if(strcmp(name, ".") == 0|| strcmp(name, "..") == 0){
            continue;
        }
        printf("%s\n", name);

        strcpy(temp, directory);
    }
    if (errno != 0){
        perror("readdir");
        exit(EXIT_FAILURE);
    }
    
    int a = closedir(dp);
    if(a == -1){
        perror("closedir");
        exit(EXIT_FAILURE);
    }
}

/** expects a path WITHOUT the / at the begining! So NO /dir/dir, only dir/dir. 
 * Returns an array of strings, with each "section" in it's own string. tokenizeBy is the character that separates sections.
 */
void tokenize_string(char* string, const char* tokenizeBy, char* input[256]){  //cd f hi -> [["cd"] ["f"] ["hi"]]
    int currOutputIndex = 0;
    char* token = strtok(string, tokenizeBy);
    int index = 0;

    for (int i = 0; token != NULL; i++){
        input[i] = token;
        token = strtok(NULL, tokenizeBy);
        index = i; 
    }
    input[index+1]= NULL;
}



int main(){

struct sigaction action = {0}; 
action.sa_handler = handle_sigint;
if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
}
checkErrorNo("sigaction");

uid_t uid = getuid();
checkErrorNo("getuid");
struct passwd *userInfo = getpwuid(uid);
checkErrorNo("getpwuid");
char* username = userInfo->pw_name;
char homeDir[300] = "/home/";
strcat(homeDir, username);
char ALThomeDir[300] = "home/";
strcat(ALThomeDir, username);


int done = 0;
    while(done != 1){
        char cwd[256];
        if (getcwd(cwd, 255) == NULL) {
        perror("getcwd");
        continue;
        }
        int wasInterupted = checkErrorNo("getcwd");
        if(wasInterupted == 1){
            continue;
        }


        printf("%s[%s]%s> ", BLUE, cwd, DEFAULT);
        /**use fgets for user input*/

        char command[1024];
        fgets(command, sizeof(command), stdin);
        wasInterupted = checkErrorNo("fgets");
        if(wasInterupted == 1){
            continue;
        }
        
        if (command){ //perform a pass to remove \n characters which can mess up tokenize!
            for (int i = 0; command[i] != '\0'; i++){
                if(command[i] == '\n'){
                    command[i] = ' ';
                }
            }
        }

        const char* seperator = " ";
        char* input[256];
        tokenize_string(command, seperator, input);
        
        int argc = 0;

        if(input){
        for (int i = 0; input[i] != NULL; i++){ //perform a pass to replace any ~'s with the home directory IF it is the ONLY argument. Doesn't detect inside of paths! and make argc!        
            if(strcmp(input[i], "~")==0){
                input[i] = homeDir;
            }
            argc++;
        }
        }

        char* action = input[0];
        

        //cd! 
        if(strcmp(action, "cd")==0){
            if(argc>2){
                fprintf(stderr, "Too many arguments.\n");
                continue;
            }

            if (argc == 1){     
                chdir(homeDir);
                checkErrorNo("chdir");
                continue;
            }
            char temp[2];
            temp[1] = '\0';
            char completepath[256] = "";

            for(int i = 0; input[1][i] != NULL; i++){ 
                if(input[1][i] == '~'){
                    if((input[1][i-1] == NULL || input[1][i-1] == '/') && (input[1][i+1] == NULL || input[1][i+1] == '/')){
                        //if true, the ~ is isolated and not part of any directory name!]
                        strcat(completepath, ALThomeDir);
                    } else {
                        temp[0] = input[1][i];
                        strcat(completepath, temp);
                    }
                } else {
                    temp[0] = input[1][i];
                    strcat(completepath, temp);
                }

            }
            chdir(completepath);
            checkErrorNo("chdir");
        }

        //exit! 
        else if(strcmp(action, "exit")==0){
            if(argc>1){
                fprintf(stderr, "Too many arguments.\n");
                continue;
            }
            exit(EXIT_SUCCESS);
        }

        //pwd! 
        else if(strcmp(action, "pwd")==0){
            if(argc>1){
                fprintf(stderr, "Too many arguments.\n");
                continue;
            }
            printf("%s\n",cwd);
        }

        else if(strcmp(action, "num")==0){
            int a[] = {1, 0, 100, 2378};
            printf("%d\n", largest_num_digits(a, 4));

        }


        //lf! **lists everything, including directories. 
        else if(strcmp(action, "lf")==0){
        if(argc>1){
                fprintf(stderr, "Too many arguments.\n");
                continue;
            }
            
        search_dirs(cwd);

        }


        //lp!
        else if(strcmp(action, "lp")==0){
            if(argc>1){
                fprintf(stderr, "Too many arguments.\n");
                continue;
            }
            int numPids = count_dirs("/proc/");
            int pids[numPids]; 
            qsortPids("/proc/", pids, numPids, cmpr_int); //put all pids into the arr. 
            lp("/proc/", 1, pids, numPids);
        }


        //exec...
        else {
            //exec
            pid_t pid;
            if((pid = fork()) < 0){
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0){ //ischild
                isChild = 1;
                execvp(input[0], input);
                checkErrorNo("execvp");
            } else {
                if (wait(NULL) == -1) {
                perror("wait");
                exit(EXIT_FAILURE);
                }
                checkErrorNo("wait");
            }
        }


        wasInterupted = checkErrorNo("");
        if(wasInterupted == 1){
            continue;
        }
        
        
    }

}