#include <utmp.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#define fifo_name "canal_cu_nume_citire_scriere" 


int login_username(char comanda[]){
    char nume_user[1024];
    int logat=0;
    FILE *ptr;
    ptr=fopen("users.txt","r");
    if(ptr==NULL)
    {
       perror("Eroare: Nu se poate deschide fisierul cu users\n");
       exit(EXIT_FAILURE);
    }
    while(fgets(nume_user, 100, ptr) != NULL)
    {
       if(strstr(comanda, nume_user) != NULL)
         {
           logat=1;
           break;
         }
       else
         {
           logat=0;
         }
    }
    fclose(ptr);
    return logat;
}

char user[32], host[256];
int32_t sec, usec;

/*
utmp mac page
#define UT_NAMESIZE      32
#define UT_HOSTSIZE     256
*/


void get_logged_users()
{
   struct utmp* entry;
   entry=getutent();
   while(entry != NULL)
   {
     strncpy(user, entry->ut_user, 32);
     strncpy(host, entry->ut_host, 256);
     sec = entry->ut_tv.tv_sec;
     usec = entry->ut_tv.tv_usec;
     entry = getutent();
   }
   
   //endutent();
}

char name[100], state[100], ppid[100], uid[100], vmsize[100];

void get_proc_info(char pid[])
{
   char inform[100];
   pid[strlen(pid)-1] = '\0';

   char fisier[100] = "/proc/";
   strcat(fisier, pid);
   strcat(fisier, "/status");

   FILE* ptr = fopen(fisier, "r");
   if(ptr == NULL)
   {
      perror("Eroare: Nu se poate deschide fisierul\n");
      exit(EXIT_FAILURE);
   }

   while((fgets(inform, 100, ptr)) != NULL)
   {
      if((strstr(inform, "Name:")) != NULL)
      {
         strncpy(name, inform, sizeof(name));
      }
      else if((strstr(inform, "State:")) != NULL)
      {
         strncpy(state, inform, sizeof(state));
      }
      else if((strstr(inform, "PPid:")) != NULL)
      {
         strncpy(ppid, inform, sizeof(ppid));
      }
      else if((strstr(inform,"Uid:")) != NULL)
      {
         strncpy(uid, inform, sizeof(uid));
      }
      else if((strstr(inform,"VmSize:")) != NULL)
      {
         strncpy(vmsize, inform, sizeof(vmsize));
      }
   }

   fclose(ptr);
}

int main()
{
   char comanda[1024];
   int fd, logat = 0, lungime_rez = 0, lungime_comanda = 0; 
   mkfifo(fifo_name,0666);
   while(1)
   {
     char rezultat[1024]="", pid[1024]="";
     fd = open(fifo_name, O_RDONLY);
     if(fd == -1)
     {
       perror("Eroare: Nu se poate deschide canalul fifo pentru citire\n");
       exit(EXIT_FAILURE);
     }

     if((read(fd, &lungime_comanda, sizeof(int))) == -1)
     {
       perror("Eroare: Nu se poate citi din fifo\n");
       exit(EXIT_FAILURE);
     }

     if((read(fd, comanda, lungime_comanda)) == -1)
     {
       perror("Eroare: Nu se poate citi din fifo\n");
       exit(EXIT_FAILURE);
     }
     close(fd);

     comanda[lungime_comanda]='\0';

     if((strstr(comanda,"login")) != NULL && logat == 0)
     {
       logat = login_username(comanda);
       int pipe1[2], p;
       if((pipe(pipe1)) == -1)
       {
         perror("Eroare: pipe1\n");
         exit(EXIT_FAILURE);
       }

       if((p = fork()) == -1)
       {
         perror("Eroare: fork pipe1\n");
         exit(EXIT_FAILURE);
       }
       else if(p == 0)//copil
       {
         close(pipe1[0]);//capat citire
         write(pipe1[1], &logat, sizeof(int));
         close(pipe1[1]);//capat scriere
         exit(0);
       }
       else
       {
         wait(NULL);
         close(pipe1[1]);
         read(pipe1[0], &logat, sizeof(int));
         close(pipe1[0]);

         fd = open(fifo_name, O_WRONLY | O_TRUNC);//deschid pt scriere si sterg continutul primit de la client
         if(fd == -1)
         {
           perror("Eroare: Nu se poate deschide canalul fifo pentru scriere\n");
           exit(EXIT_FAILURE);
         }

         if(logat == 1)
         {
            strcpy(rezultat, "Utilizator gasit\n");
         }
         else
         {
            strcpy(rezultat, "Utilizatorul nu a fost gasit\n");
         }

         lungime_rez =  strlen(rezultat);
         write(fd, &lungime_rez, sizeof(int));
         write(fd, rezultat, lungime_rez);
         close(fd);
       }
     }
     else if((strstr(comanda,"login")) != NULL && logat == 1)
     {
         fd = open(fifo_name, O_WRONLY | O_TRUNC);
         if(fd == -1)
         {
           perror("Eroare: Nu se poate deschide canalul fifo pentru scriere\n");
           exit(EXIT_FAILURE);
         }

         strcpy(rezultat, "Utilizator deja logat\n");
         lungime_rez = strlen(rezultat);
         write(fd, &lungime_rez, sizeof(int));
         write(fd, rezultat, lungime_rez);
         close(fd);
     }
     else if((strstr(comanda,"users")) != NULL && logat == 1)
     {
        int pipe2[2], p;
        if((pipe(pipe2)) == -1)
        {
          perror("Eroare: pipe2\n");
          exit(EXIT_FAILURE);
        }

        if((p = fork()) == -1)
        {
          perror("Eroare: fork pipe2\n");
          exit(EXIT_FAILURE);
        }
        else if(p == 0)
        {
          close(pipe2[0]);
          get_logged_users();

          strcat(rezultat, user);
          strcat(rezultat, "\n");
          strcat(rezultat, host);
          strcat(rezultat, "\n");

          char ssec[100], ussec[100];
          sprintf(ssec, "%d", sec);
          sprintf(ussec, "%d", usec);

          strcat(rezultat, ssec);
          strcat(rezultat, "\n");
          strcat(rezultat, ussec);
          strcat(rezultat,"\n\0");

          lungime_rez = strlen(rezultat);
          write(pipe2[1], &lungime_rez, sizeof(int));
          write(pipe2[1], rezultat, lungime_rez);
          close(pipe2[1]);
          exit(1);
        }
        else
        {
          wait(NULL);
          close(pipe2[1]);
          read(pipe2[0], &lungime_rez, sizeof(int));
          read(pipe2[0], rezultat, lungime_rez);
          close(pipe2[0]);

          fd = open(fifo_name, O_WRONLY | O_TRUNC);
          if(fd == -1)
          {
            perror("Eroare: Nu se poate deschide canalul fifo pentru scriere\n");
            exit(EXIT_FAILURE);
          }

          write(fd, &lungime_rez, sizeof(int));
          write(fd, rezultat, lungime_rez);
          close(fd); 
        }
     }
     else if((strstr(comanda, "proc")) != NULL && logat == 1)
     {
         strcpy(pid, comanda+16);
         int sockp[2], p;
         if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0)
         {
            perror("Eroare: Nu se poate crea socket-ul\n");
            exit(EXIT_FAILURE);
         }

         if((p = fork()) == -1)
         {
           perror("Eroare: fork in socket\n");
           exit(EXIT_FAILURE);
         }
         else if(p == 0)
         {
           close(sockp[0]);
           get_proc_info(pid);

           strcat(rezultat, name);
           strcat(rezultat, "\n");
           strcat(rezultat, state);
           strcat(rezultat, "\n");
           strcat(rezultat, ppid);
           strcat(rezultat, "\n");
           strcat(rezultat, uid);
           strcat(rezultat, "\n");
           strcat(rezultat, vmsize);
           strcat(rezultat, "\n\0");

           lungime_rez = strlen(rezultat);
           write(sockp[1], &lungime_rez, sizeof(int));
           write(sockp[1], rezultat, lungime_rez);
           close(sockp[1]);
           exit(2);
         }
         else
         {
           wait(NULL);
           close(sockp[1]);
           read(sockp[0], &lungime_rez, sizeof(int));
           read(sockp[0], rezultat, lungime_rez);
           close(sockp[0]);

           fd = open(fifo_name, O_WRONLY | O_TRUNC);
           if(fd == -1)
           {
              perror("Eroare: Nu se poate deschide canalul fifo pentru scriere\n");
              exit(EXIT_FAILURE);
           }

           write(fd, &lungime_rez, sizeof(int));
           write(fd, rezultat, lungime_rez);
           close(fd);
         }
     }
     else if((strstr(comanda, "logout")) != NULL && logat == 1)
     {
         logat=0;

         fd = open(fifo_name, O_WRONLY | O_TRUNC);
         if(fd == -1)
         {
           perror("Eroare: NU se poate deschide canalul fifo pentru scriere\n");
           exit(EXIT_FAILURE);
         }

         strcpy(rezultat, "Utilizator delogat\n");
         lungime_rez = strlen(rezultat);
         write(fd, &lungime_rez, sizeof(int));
         write(fd, rezultat, lungime_rez);
         close(fd);
     }
     else if((strstr(comanda, "logout")) !=NULL && logat == 0)
     {
         fd = open(fifo_name, O_WRONLY | O_TRUNC);
         if(fd == -1)
         {
            perror("Eroare: Nu se poate deschide canalul fifo pentru scriere\n");
            exit(EXIT_FAILURE);
         }

         strcpy(rezultat, "Nu exista utilizator conectat\n");
         lungime_rez = strlen(rezultat);
         write(fd, &lungime_rez, sizeof(int));
         write(fd, rezultat, lungime_rez);
         close(fd);
     }
     else if((strstr(comanda, "quit")) != NULL)
     {
        pid_t pid = getpid();
        kill(pid, SIGKILL);
     }
   }
    return 0;
}
