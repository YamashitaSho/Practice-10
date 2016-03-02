#include "header.h"

/*
server.cでやること

子スレッドを作成し、サーバプログラムとしてポート8888で待機する。
接続が確立したら同時に10個まで子スレッドを作り、接続待機状態を維持する。
clientからファイル名が送られてくるので、文字列ファイルを読み込んで内容を返信する。
*/
threadinfo_t thread_global[CONNECT_MAXMAX+1]; 

//接続状態を監視しつつ待機し、接続があれば新しい子スレッドを作る。
int server_main(){
     int i;
     int err;
     int port = atoi(DEFAULT_PORT);
     int listening_socket;
     pthread_t thread_id[cfg->max_connection+1];    //スレッド識別変数(最後の一つはエラー用)
     struct sockaddr_in peer_sin;
     socklen_t len = sizeof(struct sockaddr_in);
     
     server_setup(&listening_socket);
     printf("使用ポート:%d\n", port);
     while(1){
          for (i = 0; i < cfg->max_connection+1 ; i++){
               if (thread_global[i].state != -1){
                    continue;
               }//以降 thread_global[i].state == -1 がTRUEのときの処理
               //ステータスがヒマだった通信用ソケットを使用して接続待機
               thread_global[i].state = 0;                                      //スレッドのステータスを接続待ちに変える
               connection_number();
               thread_global[i].socket = accept( listening_socket , (struct sockaddr *)&peer_sin , &len );
               if ( thread_global[i].socket == -1){
                    error_message(ERROR_SOCKET_ACCEPT);
               }
               if ( i < cfg->max_connection){                                 //max_connectionを超えていない
                    status_send(thread_global[i].socket, STATUS_OK, STATUS_LENGTH);  //ヘッダを送る:000(エラーなし)
                    printf("接続しました:%s\n",inet_ntoa(peer_sin.sin_addr));   //クライアントのIPアドレスを表示
                    thread_global[i].state = 1;
                    pthread_create(&thread_id[i], NULL, (void *)connect_thread, &thread_global[i]); //スレッドを作る
                    pthread_detach(thread_id[i]);                                            //スレッドの終了は待たない
                    break;
               } else {                                                       //max_connectionを超えている
                    status_send(thread_global[i].socket, STATUS_SERVICE_UNAVAILABLE, STATUS_LENGTH);//ヘッダを送る:503(アクセス過多)
                    printf("接続過多により接続を拒否します:%s\n",inet_ntoa(peer_sin.sin_addr));
                    thread_global[i].state = -1;                                     //ステータスをヒマに戻す
                    err = close(thread_global[i].socket);                            //通信用ソケットを破棄
                    if (err == -1){
                         error_message(ERROR_SOCKET_CLOSE);
                    }
               }
          }//for
     }//while
     
     err = close(listening_socket);
     if (err == -1){
          error_message(ERROR_SOCKET_CLOSE);
     }
     return 0;
}
//サーバーの初期設定
int server_setup(int *listening_socket){
     int err;
     int i;
     int sock_optival = 1;
     struct addrinfo hints;                         //getaddrinfoに渡すデータ
     struct addrinfo *result;                       //getaddrinfoから受け取るデータ
     struct addrinfo *rp;
     
     for (i=0 ; i < cfg->max_connection+1 ; i++){
          thread_global[i].state = -1;              //初期化
     }
     memset(&hints,0,sizeof(hints));
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE;
     hints.ai_protocol = 0;
     
     err = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
     if (err != 0){
          printf("getaddrinfo FAILED\n");
          exit(0);
     }
     rp = result;                                   //getaddrinfoで受け取ったアドレス構造体の配列
     for (rp = result; rp != NULL; rp = rp->ai_next) { //構造体ごとにバインドを試す
          *listening_socket = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
          if (*listening_socket == -1){             //ソケットが作れなかったらスルー
               printf("a\n");
               continue;
          }
          setsockopt(*listening_socket, SOL_SOCKET, SO_REUSEADDR, &sock_optival, sizeof(sock_optival) );
          if (bind(*listening_socket, rp->ai_addr, rp->ai_addrlen) == 0){
            break;                                  //バインド成功
          }
          close(*listening_socket);                 //バインドに失敗したのでソケットを閉じる
     }
     if (rp == NULL) {                              //有効なアドレス構造体がなかった
          error_message(ERROR_SOCKET_BIND);
     }
     freeaddrinfo(result);
     //ポートを見張るようにOSに命令する
     err = listen ( *listening_socket, SOMAXCONN) ;
     if ( err == -1 ){
          error_message(ERROR_SOCKET_LISTEN);
     }
     return 0;
}
//接続数を表示 / global変数のthreadを参照しています
void connection_number(){
     int state_check;
     //thread_global[i].state(1:接続中 0:接続待機 -1:ヒマ)
     state_check = 0;
     for (int i = 0 ; i < cfg->max_connection+1 ; i++){
          if (thread_global[i].state == 1 ){
               state_check ++;
          }
//   printf("%d.",thread_global[i].state);
     }
     printf("接続数:(%d/%d)\n", state_check, cfg->max_connection);
}

//子スレッドでの処理
void connect_thread(threadinfo_t *thread){
     int err;
     
     while (1){
          err = server_receive_transmission(thread->socket);      //データのやりとり
          if (err != NO_ERROR){
               printf("クライアントがいなくなったためスレッドを破棄します\n");
               thread->state = -1;
               break;                                       //クライアントが落ち
          } 
          err = close(thread->socket);
          if (err == -1){
               error_message(ERROR_SOCKET_CLOSE);
          }
          thread->state = -1;                               //スレッドのステータスを解放済みに変える
          printf("通信を終了しました\n");
          break;
     }
     connection_number();
}

//送受信
int server_receive_transmission(int socketid){
     char *filedata;
     char **filedatap;
     char filename[256];
     char filesizehead[FILESIZE_LENGTH+1];
     int err;
     int status;
     int filesize;
     
     filedatap = &filedata;                     //load_filedataでmalloc transmission_filedataでfree
     memset(filename,'\0',256);
     
     err = receive_filename(socketid , filename);
     if ( err ){
          return err;
     }
     printf("filename:%s\n",filename);
     
     status = load_filedata(filename, filedatap, &filesize);
     status_send(socketid, status, STATUS_LENGTH);   //ファイル読み込みの結果を送信
     
     if (status == 200){
          status_send(socketid, filesize, FILESIZE_LENGTH);
          error_message( transmission_filedata(socketid , filedatap, filesize) );
     }
     return NO_ERROR;
}

//ファイル名の受信
int receive_filename(int socketid, char *filename){
     char buf[256];
     char bufc = '\0';
     
     memset(buf,'\0',256);
     
     do {
          strncat(buf , &bufc , 1);
          read(socketid , &bufc , 1);
     } while ( bufc != '\0' );
     
     if ( buf[0]== '\0' ){
          printf("ファイル名の受信エラーです\n");
          return ERROR_RECEIVE_NAME;
     }
     printf("受信しました>>%s\n",buf);
     strcpy(filename,"root/");
     strncat(filename,buf,250);
     
     return 0;
}
//ファイルの読み込み
int load_filedata(char *filename, char **filedata, int *filesize){
     char buf[256];
     FILE *fp;
     
     fp = fopen(filename, "r");
     if (fp == NULL){
          printf("FILE NOT FOUND\n");
          return STATUS_NOTFOUND;
     }
     fseek(fp, 0, SEEK_END);
     *filesize = ftell(fp);
     fseek(fp, 0, SEEK_SET);
     
     printf("filesize:%d\n", *filesize);
     *filedata = (char *)calloc(*filesize, sizeof(char));
     if ( !(*filedata) ){
          printf("PAY LOAD TOO LARGE");
          return STATUS_PAYLOADTOOLARGE;
     }
     fread(*filedata, sizeof(char), *filesize, fp);   //filedataにfilesize分ファイルを読み込む
     fclose(fp);
     return STATUS_OK;
}

//ファイルの送信
//filenameでファイル名を受け取り、filedataで返す
//filedataはここで解放する
int transmission_filedata(int socketid, char **filedata, int filesize){
     int index = 0;              //現在書き込んだ文字数
     int writesize = 0;          //書き込みサイズ
     do{
          writesize = write(socketid, *filedata+index , 1);
          if ( writesize == -1 ){
               perror(NULL);
               free(*filedata);
               return ERROR_TRANSMISSION_FAILED;
          }
          index += writesize;
     } while (index != filesize);
     printf("ファイルを送信できたようです\n");
     free(*filedata);
     return NO_ERROR;
}

//ステータスの送信 length長に右詰め0埋めフォーマットに変換して送信
int status_send(int socketid, int status, int length){
     int digit = 0;              //桁数
     int i;                      //文字列に変換する時に使うカウント変数
     int dig_status = status;    //statusの桁数を調べるためのコピー
     char *status_send;          //送られる文字列
     char *status_char;          //文字列に変換したステータス
     
     int index = 0;              //現在書き込んだ文字数
     int writesize = 0;          //書き込みサイズ
     status_send = (char *)calloc(length, sizeof(char)); //length長で0埋め
     status_char = (char *)calloc(length, sizeof(char));
     
     for (digit=0;dig_status!=0;digit++){
          dig_status = dig_status / 10;
     }
     if (digit > length){
          printf("BAD STATUS\n");
          return 1;               //桁数がフォーマット長より大きい場合はエラー
     }

     sprintf(status_char,"%d",status);     //status_charにsprintfで形式変換する
     for (i=0 ; i < digit ; i++){          //status_sendに送る形式で設定する
          status_send[digit-i-1] = status_char[digit-i-1];     //statusの一桁目から詰める
     }
     do{
          if ( (writesize = write(socketid, status_send+index , 1)) > 0){
          index++;
          }
     } while (index != length);
     //送信ステータスの表示
/*     printf("HEADER:");
     for (int i = 0;i<length;i++){
     if (status_send[i] == 0){printf("!");}
     else{printf("%c",status_send[i]);}
     }printf("\n");*/
     
     free(status_send);
     free(status_char);
     return 0;
     }