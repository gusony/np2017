#define COMMAND_NUM 256
#define MESSAGE_LEN 10000
#define SERV_TCP_PORT 7575
#define total_command_number 5000

typedef struct command{
    char   *com_str[50];
    int    para_len;
    int    output_to;
    int    output_to_bytes;
    char   output_file[30];
    int    output_line;
}command_t;
