#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <string.h>


// int manager_pid;

int cpu_pipefd[2];
int mem_pipefd[2];
int fsw_pipefd[2];
int fsr_pipefd[2];

int cpu_out;
int mem_out;
int fsw_out;
int fsr_out;

void now_string(char * time_string) { 
    time_t now_time;
    struct tm *timeinfo;

    now_time = time(NULL);
    timeinfo = localtime(&now_time);
    strftime(time_string, 9, "%H:%M:%S", timeinfo);
}


void *cpu_mg(){

    float result;
    int user_time, nice_time, syteam_time, idle_time; // TODO: ina float
    FILE * process_file;


    while (1){
        char *format = "<mg name>cpu</mg name><mg content>%d</mg content><mg timestamp>%s</mg timestamp>";
        char msg[110];
        
        process_file = fopen("/proc/stat", "r");
        fscanf(process_file, "%*s %d %d %d %d", &user_time, &nice_time, &syteam_time, &idle_time);
        fclose(process_file);

        result = user_time + nice_time + syteam_time;
        result = (result / (user_time + nice_time + syteam_time + idle_time)) * 100;

        char now_time[9];
        now_string(now_time);
        sprintf(msg, format, (int) result, now_time);
        write(cpu_pipefd[1], msg, strlen(msg));

    }

}

void *mem_mg(){
    char *format = "<mg name>mem</mg name><mg content>%d</mg content><mg timestamp>%s</mg timestamp>";
    char msg[110];
    float total, free;
    float mem_usage;
    FILE *fp;

    while (1) {
        fp = fopen("/proc/meminfo", "r");
        fscanf(fp, "MemTotal: %f kB\nMemFree: %f", &total, &free);
        fclose(fp);

        mem_usage = ((total - free) / (total)) * 100;

        char now_s[9];
        now_string(now_s);
        sprintf(msg, format, (int) mem_usage, now_s);
        write(mem_pipefd[1], msg, strlen(msg));
    }

}

void *fsw_mg(){
    char *format = "<mg name>fsw</mg name><mg content>%d</mg content><mg timestamp>%s</mg timestamp>";
    char msg[110];
    float write_sectors, write_duration;
    float fsw;
    FILE *fp;

    int c = 0;
    while (1) {
        fp = fopen("/sys/block/sda/stat", "r");
        fscanf(fp, "%*d %*d %*d %*d %*d %*d %f %f", &write_sectors, &write_duration);
        fclose(fp);

        fsw = (write_sectors * 512) / (write_duration / 1000);

        char now_s[9];
        now_string(now_s);
        sprintf(msg, format, (int) fsw, now_s);
        write(fsw_pipefd[1], msg, strlen(msg));
  }
}

void *fsr_mg(){
    char *format = "<mg name>fsr</mg name><mg content>%d</mg content><mg timestamp>%s</mg timestamp>";
    char msg[110];
    float read_sectors, read_duration;
    float fsr;
    FILE *fp;

    while (1) {
        fp = fopen("/sys/block/sda/stat", "r");
        fscanf(fp, "%*d %*d %f %f", &read_sectors, &read_duration);
        fclose(fp);
        fsr = (read_sectors * 512) / (read_duration / 1000);
        char now_s[9];
        now_string(now_s);
        sprintf(msg, format, (int) fsr, now_s);
        write(fsr_pipefd[1], msg, strlen(msg));
    }
}

void alarm_handler(int sig) {
  if (sig == SIGALRM) {
    char now_s[9];
    now_string(now_s);
    printf("Cpu: %d\nMem: %d\nFsw: %d\nFsr: %d\nTime: %s\n", cpu_out, mem_out, fsw_out, fsr_out, now_s);
    alarm(5);
  }
}

int main(){
    
    int pid = fork();

    if (pipe(cpu_pipefd) == -1) {
        perror("Cpu pipe");
        exit(1);
    }

    if (pipe(mem_pipefd) == -1) {
        perror("Mem pipe");
        exit(1);
    }

    if (pipe(fsw_pipefd) == -1) {
        perror("Fsw pipe");
        exit(1);
    }

    if (pipe(fsr_pipefd) == -1) {
        perror("Fsr pipe");
        exit(1);
    }

    if (pid == 0){
    
        pthread_t thread1, thread2, thread3, thread4;

        int manager_pid = getppid();

        close(cpu_pipefd[0]);
        close(mem_pipefd[0]);
        close(fsw_pipefd[0]);
        close(fsr_pipefd[0]);

        pthread_create(&thread1, NULL, cpu_mg, NULL);
        pthread_create(&thread2, NULL, mem_mg, NULL);
        pthread_create(&thread3, NULL, fsw_mg, NULL);
        pthread_create(&thread4, NULL, fsr_mg, NULL);
    
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
        pthread_join(thread3, NULL);
        pthread_join(thread4, NULL);

        close(cpu_pipefd[1]);
        close(mem_pipefd[1]);
        close(fsw_pipefd[1]);
        close(fsr_pipefd[1]);

        // pthread_create();

        printf("%s\n", "fuck you");
    }else if (pid > 0){
                if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
                  perror("Unable to catch alarm signal.");
                  exit(1);
                }
                alarm(5);

                close(cpu_pipefd[1]);
                close(mem_pipefd[1]);
                close(fsw_pipefd[1]);
                close(fsr_pipefd[1]);

                struct timeval select_timeout;
                int nfds = 0;
                nfds = nfds > cpu_pipefd[0] ? nfds : cpu_pipefd[0];
                nfds = nfds > mem_pipefd[0] ? nfds : mem_pipefd[0];
                nfds = nfds > fsw_pipefd[0] ? nfds : fsw_pipefd[0];
                nfds = nfds > fsr_pipefd[0] ? nfds : fsr_pipefd[0];
                int res;

                char buf;
                char cpu_buf[110];
                char mem_buf[110];
                char fsw_buf[110];
                char fsr_buf[110];

                int cpu_eof = 0;
                int mem_eof = 0;
                int fsw_eof = 0;
                int fsr_eof = 0;

                int c = 0;
                while (c < 50) {
                  c++;
                  fd_set read_fds;
                  FD_ZERO(&read_fds);
                  FD_SET(cpu_pipefd[0], &read_fds);
                  FD_SET(mem_pipefd[0], &read_fds);
                  FD_SET(fsw_pipefd[0], &read_fds);
                  FD_SET(fsr_pipefd[0], &read_fds);

                  select_timeout.tv_sec = 5;
                  select_timeout.tv_usec = 0;

                  memset(cpu_buf, 0, 110);
                  memset(mem_buf, 0, 110);
                  memset(fsw_buf, 0, 110);
                  memset(fsr_buf, 0, 110);

                  res = select(nfds + 1, &read_fds, NULL, NULL, &select_timeout);

                  if (res == -1) {
                    if (errno == EINTR) {
                      continue;
                    } else {
                      perror("select");
                      exit(1);
                    }
                  } else if (res) {
                    int r;
                    if (!cpu_eof) {
                      if (FD_ISSET(cpu_pipefd[0], &read_fds)) {
                        int pos = 0;
                        while (1) {
                          r = read(cpu_pipefd[0], &buf, 1);
                          if (!r) {
                            cpu_eof = 1;
                            break;
                          }
                          if (buf == '>'){
                            char tag[13];
                            strcpy(tag, &cpu_buf[pos - 14]);
                            if (!strcmp(tag, "</mg timestamp")) {
                              cpu_buf[pos] = buf;
                              break;
                            }
                          }
                          cpu_buf[pos] = buf;
                          pos++;
                        }
                        puts(cpu_buf);
                        sscanf(cpu_buf + 34, "%d", &cpu_out);
                      }
                    }
                    buf = 0;
                    if (!mem_eof) {
                      if (FD_ISSET(mem_pipefd[0], &read_fds)) {
                        int pos = 0;
                        while (1) {
                          r = read(mem_pipefd[0], &buf, 1);
                          if (!r) {
                            mem_eof = 1;
                            break;
                          }
                          if (buf == '>'){
                            char tag[13];
                            strcpy(tag, &mem_buf[pos - 14]);
                            if (!strcmp(tag, "</mg timestamp")) {
                              mem_buf[pos] = buf;
                              break;
                            }
                          }
                          mem_buf[pos] = buf;
                          pos++;
                        }
                        puts(mem_buf);
                        sscanf(mem_buf + 34, "%d", &mem_out);
                      }
                    }
                    buf = 0;
                    if (!fsw_eof) {
                      if (FD_ISSET(fsw_pipefd[0], &read_fds)) {
                        int pos = 0;
                        while (1) {
                          r = read(fsw_pipefd[0], &buf, 1);
                          if (!r) {
                            fsw_eof = 1;
                            break;
                          }
                          if (buf == '>'){
                            char tag[13];
                            strcpy(tag, &fsw_buf[pos - 14]);
                            if (!strcmp(tag, "</mg timestamp")) {
                              fsw_buf[pos] = buf;
                              break;
                            }
                          }
                          fsw_buf[pos] = buf;
                          pos++;
                        }
                        puts(fsw_buf);
                        sscanf(fsw_buf + 34, "%d", &fsw_out);
                      }
                    }
                    buf = 0;
                    if (!fsr_eof) {
                      if (FD_ISSET(fsr_pipefd[0], &read_fds)) {
                        int pos = 0;
                        while (1) {
                          r = read(fsr_pipefd[0], &buf, 1);
                          if (!r) {
                            fsr_eof = 1;
                            break;
                          }
                          if (buf == '>'){
                            char tag[13];
                            strcpy(tag, &fsr_buf[pos - 14]);
                            if (!strcmp(tag, "</mg timestamp")) {
                              fsr_buf[pos] = buf;
                              break;
                            }
                          }
                          fsr_buf[pos] = buf;
                          pos++;
                        }
                        puts(fsr_buf);
                        sscanf(fsr_buf + 34, "%d", &fsr_out);
                      }
                    }
      } else {
        printf("Metric gatherers took too long.");
      }
    }



        printf("%s\n", "fuck me");
    }else {
        printf("%s\n", "fuck all thing in the world");
    }
    return 0;
}