#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#define  fifo_name "canal_cu_nume_citire_scriere"

int main(){

   int fd, lungime_rez = 0, lungime_comanda = 0; 
   char comanda[1024];
   mkfifo(fifo_name, 0666);
   while(1)
   {
     char rezultat[1024] = "";
     fgets(comanda, 1024, stdin);
     lungime_comanda = strlen(comanda);
     fd=open(fifo_name, O_WRONLY | O_TRUNC);
     if(fd == -1)
     {
        perror("Eroare: Nu se poate deschide canalul fifo pentru scriere\n");
        exit(EXIT_FAILURE);
     }

     if((write(fd, &lungime_comanda, sizeof(int))) == -1)
     {
        perror("Eroare: Nu se poate scrie in fifo\n");
        exit(EXIT_FAILURE);
     }

     if((write(fd, comanda, lungime_comanda)) == -1)
     {
        perror("Eroare: Nu se poate scrie in fifo\n");
        exit(EXIT_FAILURE);
     }
     close(fd);

     if(strstr(comanda, "login") != NULL || strstr(comanda, "users") != NULL || strstr(comanda, "proc") != NULL || strstr(comanda, "logout") != NULL || strstr(comanda, "quit") != NULL)
     {
       if(strstr(comanda, "quit") != NULL)
       {
         pid_t pid = getpid();
         kill(pid, SIGKILL);
       }

       fd = open(fifo_name, O_RDONLY);
       if(fd == -1)
       {
          perror("Eroare: Nu se poate deschide canalul fifo pentru citire\n");
          exit(EXIT_FAILURE);
       }

       if((read(fd, &lungime_rez, sizeof(int))) < 0)
       {
          perror("Eroare: Nu se poate citi din fifo\n");
          exit(EXIT_FAILURE);
       }

       if((read(fd, rezultat, lungime_rez)) < 0)
       {
         perror("Eroare: Nu se poate citi din fifo\n");
         exit(EXIT_FAILURE);
       }
       close(fd);

       if((write(1, rezultat, lungime_rez)) < 0)
       {
         perror("Eroare: Nu se poate scrie in terminal\n");
         exit(EXIT_FAILURE);
       }

     }
     else
     {
        strcpy(rezultat, "Comanda necunoscuta\n");
        lungime_rez = strlen(rezultat);
        write(1, rezultat, lungime_rez);
     }
   }
   return 0;
}
