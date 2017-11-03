#ifndef COMMON_H

#define COMMON_H

#define CLIENT_NUMBERS 30
#define CMD_LENGTH 15000
#define CMDS_SIZE 5000
#define BUFFER_SIZE 1024
#define PIPE_SIZE 1001
#define BIN_FILES_SIZE 20
#define PUBLIC_PIPE_SIZE 101
#define IP_LENGTH 16
#define PORT_LENGTH 6
#define NAME_LENGTH 21

#define DEFAULT_PATH "bin:."
#define DEFAULT_DIR "ras"
#define DEFAULT_NAME "(no name)"

#define SHM_CLIENT_TO_SERVER_CMD 31464
#define SHM_CLIENT_TO_SERVER_PARA 31465
#define SHM_CLIENT_TO_SERVER_PARA2 31466
#define SHM_CLIENT_TO_SERVER_PARA3 31467

#define SHM_FD_KEY 18147
#define SHM_IP_KEY 18148
#define SHM_PORT_KEY 18149
#define SHM_NAME_KEY 18150

void doubleArray(char **arr, int &size);
void split(char **arr, int &len, char *str, const char del);
void newSplit(char arr[CMDS_SIZE][BUFFER_SIZE], int &len, char *str, const char *del);
int readline(int fd, char *str, int maxlen);
bool startWith(const char *pre, const char *str);
void closePipe(int pipes[][2], int pipeSize);
int getBinFile(char **files, int &count, int &size);

int getShmID(char *key, int no, int size);
int getShmID(key_t key, int size);

void errexit(const char *message);
void errexit(const char *message, const char format[]);
void errexit(const char *message, const int format1);
void errexit(const char *message, const char *format1);
void errexit(const char *message, const char *format1,  const char *format2);
void errexit(const char *message, const int format1,  const char *format2);
void errexit(const char *message, const char *format1, const char *format2, const char *format3);
void errexit(const char *message, const char *format1, const int format2, const char *format3);
void errexit(const char *message, const int format1, const int format2, const char *format3);

#endif
