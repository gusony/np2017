#ifndef COMMON_H

#define COMMON_H

#define CLIENT_NUMBERS 30
#define CMD_LENGTH 15000
#define CMDS_SIZE 5000
#define BUFFER_SIZE 256
#define PIPE_SIZE 1001
#define BIN_FILES_SIZE 20
#define PUBLIC_PIPE_SIZE 100
#define NAME_LENGTH 20

#define DEFAULT_PATH "bin:."
#define DEFAULT_DIR "ras"
#define DEFAULT_NAME "(no name)"

void doubleArray(char **arr, int &size);
void split(char **arr, int &len, char *str, const char del);
int readline(int fd, char *str, int maxlen);
bool startWith(const char *pre, const char *str);
void closePipe(int *pipes, int pipeSize);
int getBinFile(char **files, int &count, int &size);

void errexit(const char *message);
void errexit(const char *message, const char format[]);
void errexit(const char *message, const int format1);
void errexit(const char *message, const char *format1);
void errexit(const char *message, const char *format1,  const char *format2);
void errexit(const char *message, const char *format1, const char *format2, const char *format3);

#endif
