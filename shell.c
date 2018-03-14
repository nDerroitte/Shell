/* --------------------------------------------------------------------------------------*\
 * Brieven Geraldine - Derroitte Natan
 *
 * Operating System - Project 2 - 02.03.17
 *
 * shell.c
 \* --------------------------------------------------------------------------------------*/


/*struct pfstat {
    int stack_low; //Number of times the stack was expanded after a page fault
    int transparent_hugepage_fault; //Number of huge page faulttransparent PMD page
    
    int anonymous_fault; //Normal anonymous page fault
    int file_fault; //Normal file-backed page fault
    int swapped_back; //Number of fault that produced a read-from swap to put
    back the page online
    int copy_on_write; //Normal of fault which backed a copy-on-write;
    int fault_alloced_page; //Number of normal pages allocated due to a page
    fault (no matter the code path if it was for an anonymous fault, a cow, ...). }*/





#define FIRST_OCCURENCE 0
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdbool.h>

//------------------------------- Function prototypes -----------------------------------
char* mystrdup (const char *currentString);
bool findInFile (const char* path, char* string, char* outputString, int cpuNb);
bool replaceInFile (const char* path, int cpuFreq);
void detectDollar(char** arguments, int size, int precReturn, int precPID);


/* --------------------------------------------------------------------------------------*\
 * mystrdup
 *
 * Allocate memory for a new string which is a copy of the input one
 *
 * INPUTS  : String to copy
 * OUTPUT :  Copied string
 \* --------------------------------------------------------------------------------------*/
char* mystrdup (const char *currentString)
{
    char *newString = malloc (strlen (currentString) + 256);
    if (newString == NULL)
        return NULL;
    strcpy (newString,currentString);
    return newString;
}

/* --------------------------------------------------------------------------------------*\
 * findInFile
 *
 * Search in a file the rest of the line after the nb occurence of a string
 *
 * INPUTS  :  - the path of the file
 *            - the string to look for
 *            - the string where to write the rest of the line
 *            - the cpu number representing the occurence of the string we search
 * OUTPUT :  - true if no problem occurs
 *           - false otherwise
 \* --------------------------------------------------------------------------------------*/
bool findInFile (const char* path, char* string, char* outputString, int cpuNb)
{
    FILE *file = fopen(path, "rb");
    if(file == NULL)
    {
        perror("Error opening file");
        return false;
    }
    if(!strcmp(string,"hostname"))
    {
        fgets(outputString, 256, file);
        fclose(file);
        return true;
    }
    int size = (int)strlen(string);
    fgets(outputString, size+1, file);
    for(int i =0; i<(cpuNb+1);i++)
    {
        while(strncmp(outputString,string, size))
        {
            if(fgets(outputString, size+1, file)==NULL)
            {
                strcpy(outputString, "");
                return false;
            }
        }
        fgets(outputString, 256, file);
    }
    fclose(file);
    return true;
}

/* --------------------------------------------------------------------------------------*\
 * replaceInFile
 *
 * Creates a new file having the same content of the given file, except the frequency
 * which is replaced
 *
 * INPUTS  :  - the path of the file
 *            - the frequency which has to be set
 * OUTPUT :   - true if no problem occurs
 *            - false otherwise
 \* --------------------------------------------------------------------------------------*/
bool replaceInFile (const char* path, int cpuFreq)
{
    FILE* fd = NULL;
    fd = fopen(path,"wb");
    if (fd == NULL)
    {
        printf("Error opening the file : the cpu doesn't exist or you are not executing this command in super user mode.\n");
        return false;
    }
    fprintf(fd, "%d",  cpuFreq);
    fclose(fd);
    return true;
}


void detectDollar(char** arguments, int size, int precReturn, int precPID)
{
    // Check $
    
    for(int i=0;i<size;++i){
       
        if(!strcmp(arguments[i], "$!")){
            if(precPID!=0)
                sprintf(arguments[i], "%d",precPID);
            else
                strcpy(arguments[i],"");
        }
           
        else if (!strcmp(arguments[i], "$?")){
//            waitpid(-1, &precReturn, WNOHANG);
            sprintf(arguments[i], "%d", precReturn);
        }
    }
    
}

/* --------------------------------------------------------------------------------------*\
 * main.c
 *
 * Implementation of a shell without build in function
 \* --------------------------------------------------------------------------------------*/
int main()
{
    int nbPaths=0,i=0 ,j=0, waitstatus, cpuNb =1, inputCpuFreq = 0;
    int socketFD=0, precReturn=0, precPID = 0;
    bool end = false;
    char* arguments [256];
    char* pathArray [20];
    char cmd [256],currDirectory[256], outputString [256], line [65536];
    char* tmp;
    struct ifreq iFreq;
    bool isBackground = false;
    
    //Storage of all paths
    
    char* fullPath = getenv("PATH");
    tmp = strtok(fullPath, ":");
    
    while( tmp != NULL )
    {
        pathArray[nbPaths++]= tmp;
        tmp = strtok(NULL, ":");
    }
    
    //Interaction with the user
    
    while (!end)
    {
        //Clearing all the variables
        memset(arguments, 0, sizeof (arguments));
        strcpy(currDirectory, "");
        strcpy(cmd, "");
        strcpy(line, "");
        strcpy(outputString, "");
        j=0;
        isBackground= false;
        
        //Prompt
        printf("> ");
        fflush (stdout);
        
        //--------------------Reading command---------------------------------
        
        //CASE 1: The user wants to exit
        
        if(fgets(line,sizeof(line),stdin)==NULL || !strcmp(line,"exit\n"))
        {
            end = true;
            break;
        }
        if(!strcmp(line,"\n"))
            continue;
        
        
        //CASE 2: The user enters a command
        
        //Splitting arguments string into words
        tmp = strtok(line, "\n");
        tmp = strtok(line, " ");
        while( tmp != NULL )
        {
            arguments[j++]= tmp;
            tmp = strtok(NULL, " ");
        }
        strcpy(cmd, arguments[0]);
        
        /*---------------------------------------
         * Check if the command is followed by &
         *--------------------------------------*/
        
        if (!strcmp(arguments[j-1],"&")){
            isBackground = true;
            arguments[--j]=NULL;
        }
        
        
        /*---------------------------------------
         * Appel de fonction comme en première année
         *--------------------------------------*/
        
         detectDollar(arguments,j, precReturn, precPID);
        
        //CASE 2.1 : the command is cd
        
        if (!strcmp(cmd, "cd"))
        {
            strcpy(currDirectory,getcwd(NULL,0));
            
            //CASE 2.1.1 : cd is alone
            if(arguments[1] == NULL)
                arguments[1] = getenv("HOME");
            
            //CASE 2.1.2 : cd "a b"
            else if (arguments[1][0] =='"')
            {
                arguments[1] =strtok(arguments[1],"\"");
                i=1;
                strcat(currDirectory, "/");
                while(arguments[i]!=NULL)
                {
                    //Removing last "
                    if (arguments[i][strlen(arguments[i])-1]=='"')
                        arguments[i][strlen(arguments[i])-1]=0;
                    //We don't add a space before the first argument
                    if (i!=1)
                        strcat(currDirectory, " ");
                    strcat(currDirectory,arguments[i++]);
                }
                //if the user enter cd ""
                if (arguments[1]!=NULL)
                    strcpy(arguments[1], currDirectory);
                else
                    arguments[1] = getenv("HOME");
            }
            
            //CASE 2.1.3 : cd a\ b
            else if(arguments[1][strlen(arguments[1])-1]== '\\')
            {
                i=1;
                strcat(currDirectory, "/");
                while(arguments[i]!=NULL)
                {
                    //Removing last "
                    if (arguments[i][strlen(arguments[i])-1]=='\\')
                        arguments[i][strlen(arguments[i])-1]=0;
                    //We don't add a space before the first argument
                    if (i!=1)
                        strcat(currDirectory, " ");
                    strcat(currDirectory,arguments[i++]);
                }
                strcpy(arguments[1], currDirectory);
            }
            
            //CASE 2.1.4 : cd without a path
            else if (arguments[1][0]!='/')
            {
                strcat(currDirectory, "/");
                strcat(currDirectory,arguments[1]);
                strcpy(arguments[1], currDirectory);
            }
            
            //CASE 2.1.5 : cd ..
            if(!strcmp(arguments[1],".."))
            {
                char* newDirectory;
                newDirectory =strrchr(currDirectory,'/');
                if((newDirectory != NULL ))
                    *newDirectory = '\0';
                
                strcpy(arguments[1], currDirectory);
            }
            
            
            
            printf("%d",chdir(arguments[1]));
            continue;
        }
        
        // CASE 2.2 : the cmd is a SYS cmd (built in)
        
        else if (!strcmp(cmd, "sys"))
        {
            // CASE 2.2.1 : hostname
            if ((arguments[1]!=NULL)&&(!strcmp(arguments[1], "hostname")))
            {
                findInFile("/proc/sys/kernel/hostname", "hostname",outputString, FIRST_OCCURENCE);
                if (!strcmp(outputString,""))
                {
                    printf("1");
                    continue;
                }
                printf("Hostname:  %s0",outputString);
                continue;
            }
            
            // CASE 2.2.2 : cpu model
            if ((arguments[1]!=NULL)&&(arguments[2]!=NULL)&&(!strcmp(arguments[1], "cpu"))&&(!strcmp(arguments[2], "model")))
            {
                findInFile("/proc/cpuinfo", "model name",outputString, FIRST_OCCURENCE);
                if (!strcmp(outputString,""))
                {
                    printf("1");
                    continue;
                }
                char* newOutputString = strrchr(outputString, ':');
                printf("Cpu model %s0",newOutputString);
                continue;
            }
            
            // CASE 2.2.3 : Show cpu freq of nth processor.
            if ((arguments[1]!=NULL)&&(arguments[2]!=NULL)&&(!strcmp(arguments[1], "cpu"))&&(!strcmp(arguments[2], "freq"))&&(arguments[3]!= NULL)&&(arguments[4]==NULL))
            {
                cpuNb = atoi(arguments[3]);
                if (!findInFile("/proc/cpuinfo", "cpu MHz",outputString, cpuNb))
                {
                    printf("1");
                    continue;
                }
                char* newOutputString = strrchr(outputString, ':');
                printf("Cpu frequency (Mhz) %s0",newOutputString);
                continue;
            }
            
            // CASE 2.2.4 : Set cpu freq of nth processor.
            else if ((arguments[1]!=NULL)&&(arguments[2]!=NULL)&&(!strcmp(arguments[1], "cpu"))&&(!strcmp(arguments[2], "freq"))&&(arguments[3]!= NULL)&&(arguments[4]!=NULL))
            {
                cpuNb = atoi(arguments[3]);
                inputCpuFreq = atoi(arguments[4]);
                if (inputCpuFreq <=0)
                {
                    printf("1");
                    continue;
                }
                //Transforming it into kHz
                inputCpuFreq/= 1000;
                sprintf(outputString, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",cpuNb);
                if(!replaceInFile(outputString, inputCpuFreq))
                {
                    printf("1");
                    continue;
                }
                printf("0");
                continue;
            }
            
            // CASE 2.2.5 : Get the ip and mask of the interface DEV.
            else if ((arguments[1]!=NULL)&&(arguments[2]!=NULL)&&(!strcmp(arguments[1], "ip"))
                     &&(!strcmp(arguments[2], "addr"))&&(arguments[3]!= NULL)&&(arguments[4]==NULL))
            {
                // Creation of an endpoint for communication using UDP in datagram mode. Could have use PF_INET
                if ((socketFD= socket(AF_INET, SOCK_DGRAM,0))==-1)
                {
                    printf("Socket can't be created \n1");
                    continue;
                }
                else
                {
                    memset( &iFreq, 0, sizeof( struct ifreq ) );
                    strncpy(iFreq.ifr_name, arguments[3], IFNAMSIZ); //arguments[3] represents DEV
                    //iPV4
                    iFreq.ifr_addr.sa_family = AF_INET;
                    
                    // Getting of the corresponding IP address and mask
                    if(ioctl(socketFD, SIOCGIFADDR, &iFreq)!=-1)
                    {
                        // Conversion and display of the result
                        struct sockaddr_in* ipAddr = (struct sockaddr_in*)&iFreq.ifr_addr;
                        printf("IP address: %s\n",inet_ntoa(ipAddr->sin_addr));
                        //If we have an IP, we'll have a mask.
                        ioctl(socketFD, SIOCGIFNETMASK, &iFreq);
                        // Conversion and display of the result
                        struct sockaddr_in* mask = (struct sockaddr_in*)&iFreq.ifr_addr;
                        printf("Mask: %s\n0",inet_ntoa(mask->sin_addr));
                        close(socketFD);
                        continue;
                    }
                    else
                    {
                        printf("1");
                        continue;
                    }
                }
                
            }
            
            // CASE 2.2.6 : Set the ip and mask of the interface DEV.
            else if ((arguments[1]!=NULL)&&(arguments[2]!=NULL)&&(!strcmp(arguments[1], "ip"))&&(!strcmp(arguments[2], "addr"))&&(arguments[3]!= NULL)&&(arguments[4]!=NULL)&&(arguments[5]!=NULL))
            {
                // Creation of an endpoint for communication using UDP in datagram mode. Could have use PF_INET
                if ((socketFD= socket(AF_INET ,SOCK_STREAM,0))==-1)
                {
                    printf("Socket can't be created \n1");
                    continue;
                }
                else
                {
                    /* Notice that: - arguments[3] represents DEV
                     *              - arguments[4] represents the new ip
                     *              - arguments[5] represents the new mask */
                    
                    // Definition of the structure used to configure network devices
                    strncpy(iFreq.ifr_name, arguments[3], IFNAMSIZ); // get interface name
                    struct sockaddr_in* ipAddr = (struct sockaddr_in*)&iFreq.ifr_addr;
                    iFreq.ifr_addr.sa_family = AF_INET;
                    
                    // Conversion into a network address structure
                    inet_pton(AF_INET, arguments[4],  &ipAddr->sin_addr);
                    
                    // Setting of the new IP address
                    if (ioctl(socketFD, SIOCSIFADDR, &iFreq)!=-1)
                    {
                        
                        // Conversion into a network address structure
                        inet_pton(AF_INET, arguments[5],  &ipAddr->sin_addr);
                        
                        // Setting of the new mask
                        ioctl(socketFD, SIOCSIFNETMASK, &iFreq);
                        
                        // Setting of the flags
                        ioctl(socketFD, SIOCGIFFLAGS, &iFreq);
                        iFreq.ifr_flags |= IFF_UP | IFF_RUNNING;
                        ioctl(socketFD, SIOCSIFFLAGS, &iFreq);
                        close(socketFD);
                    }
                    else
                    {
                        printf("Error opening the file : the DEV doesn't exist or you are not executing this command in super user mode.\n1");
                        continue;
                    }
                }
            }

            
            // CASE 2.2.7 : Show the pfstat or set the pfstat mode
            else if ((arguments[1]!=NULL)&&(arguments[2]!=NULL)&&(!strcmp(arguments[1], "pfstat")))
            {
                struct pfstat* pfstat = malloc(sizeof(struct pfstat*));
                syscall(377,atoi(arguments[1]),pfstat); // Nb of our system call  getpid()
                free(pfstat);
            }
            else
            {
                printf("1");
                continue;
            }
        }
        
        // CASE 3 : the command isn't a built-in command
        
        // Duplication of the process
        pid_t pid = fork();
        if (pid  == -1)
            perror("Fork error.");
        else if (pid == 0)
        {
            // CASE 3.1 : Path is unknowns
            
            if(strchr(cmd,'/')==NULL)
            {
                for(int k=0; k<nbPaths;k++)
                {
                    if(k!=0 && arguments[0] != NULL)
                        free(arguments[0]);
                    arguments[0] = mystrdup(pathArray[k]);
                    strcat(arguments[0],"/");
                    strcat(arguments[0],cmd);
                    execv(arguments[0], arguments);
                }
                free(arguments[0]);
                exit(1);
            }
            
            
            // CASE 3.2 : We know the path
            
            else
            {
                execv(arguments[0], arguments);
                exit(1);
            }
        }
        
        // Command's execution's feedback
        
        else
        {
            if(!isBackground)
            {
                wait(&waitstatus);
                precReturn = WEXITSTATUS(waitstatus);
                printf("%d", precReturn);
            }
            else{
                printf("Pid number : %d\n",getpid());
                precPID = getpid();
            }
            
        }
    }
    return 0;
}
