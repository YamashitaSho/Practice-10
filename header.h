#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <pthread.h>

#define CONFIG_FILE "tcwebngin.conf"
#define DIRECTORY "root/"
#define FILENAME_KARI "root/tcusrs-00.csv"
#define SAVE_DIRECTORY "local"

#define CONFIG_SYMBOL_HOST "host"
#define CONFIG_SYMBOL_MAXC "max_connection"
#define DEFAULT_PORT "8888"
#define CONNECT_MAX 10                  //デフォルト最大接続数
#define CONNECT_MAXMAX 100              //最大最大接続数
#define CONNECT_MAXMIN 1                //最小最大接続数
#define FILESIZE_LENGTH 9               //ヘッダにおけるファイルサイズの桁数

#define STATUS_LENGTH 4
#define STATUS_OK 200                   //OK
#define STATUS_PAYLOADTOOLARGE 413      //処理できないデータ量のリクエストである
#define STATUS_NOTFOUND 404             //NOTFOUND
#define STATUS_SERVICE_UNAVAILABLE 503  //503エラー（接続過多)

#define NO_ERROR 0
#define ERROR_ARG_UNKNOWN 1001          //不明な引数
#define ERROR_ARG_TOO_MANY 1002         //引数大杉
#define ERROR_ARG_SEVERAL_MODES 1003    //指定モードが多い

#define ERROR_SOCKET 2001               //ソケット作成失敗
#define ERROR_SOCKET_OPTION 2002        //オプション設定失敗
#define ERROR_SOCKET_BIND 2003          //bind失敗
#define ERROR_SOCKET_LISTEN 2004        //listen失敗
#define ERROR_SOCKET_ACCEPT 2005        //接続受付失敗
#define ERROR_SOCKET_CLOSE 2007         //ソケットクローズ失敗

#define ERROR_CONNECT 2008              //接続エラー
#define ERROR_HOST_UNKNOWN 2009         //ホスト名が解決できない
#define CONFIG_NOTFOUND 2010            //コンフィグファイルがない
#define ERROR_RECEIVE_NAME 2010         //ファイル名が受信できない
#define ERROR_TRANSMISSION_FAILED 2011  //ファイル送信失敗
#define ERROR_OUTOFMEMORY 2012          //メモリ不足によるファイル受信失敗

#define ERROR_FILENOTFOUND 3001         //ファイルが見つからない

#define CLIENT_GET '1'                  //クライアントモード ファイル取得
#define CLIENT_CONFIG '2'               //設定
#define CLIENT_EXIT '9'                 //終了


#define BUF_LEN 256                     //通信バッファサイズ

struct _commandline {
    int server;
    int client;
};

struct _config {
    char host[120];
    int max_connection;
};

struct _threadinfo{
    int socket;
    int state;
};

typedef struct _commandline commandline_t;
typedef struct _config config_t;
typedef struct _threadinfo threadinfo_t;

//グローバル変数
extern threadinfo_t thread[CONNECT_MAXMAX+1];
extern config_t config;
extern config_t *cfg;


//グローバル変数ここまで

//main.c

int mode_check(char *argv[], commandline_t *commandline,int argc);
int arg_check(char* arg);
void error_message(int err);


//server.c
int server_main();
int server_setup(int *listening_socket);
void connection_number();
void connect_thread(threadinfo_t *thread);

int server_receive_transmission(int socketid);
int receive_filename(int socketid, char *filename);
int load_filedata(char *filename, char **filedata, int *filesize);
int transmission_filedata(int socketid, char **filedata, int filesize);
int status_send(int socketid, int status, int length);

//client.c
int client_main(int debugmode);
char get_menu();
void input_a_line(char *inputline);

int client_connect(config_t *cfg);
int address_resolution(struct in_addr *servhost);

int client_receive_transmission(int connecting_socket);
int transmission_filename(int socketid, char *filename);
int receive_status(FILE *fp, int length);
int receive_filedata(FILE *fp, char **filedata, int filesize, char *filename);

int file_save(char **receive_data, int filesize, char *filename);
void dir_make(char *dir);

//config.c
int config_load();
int config_param(char *configfile, char *symbolname);
int new_config();

#endif