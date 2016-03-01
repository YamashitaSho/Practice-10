#include "header.h"

/*
server.cでやること

子スレッドを作成し、サーバプログラムとしてポート8888で待機する。
接続が確立したら同時に10個まで子スレッドを作り、接続待機状態を維持する。
clientからファイル名が送られてくるので、文字列ファイルを読み込んで内容を返信する。
*/
threadinfo_t thread[CONNECT_MAXMAX+1]; 

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
          connection_number(thread);
          for (i = 0; i < cfg->max_connection+1 ; i++){
               if (thread[i].state == -1){
                    //ステータスがヒマだった通信用ソケットを使用して接続待機
                    thread[i].socket = accept( listening_socket , (struct sockaddr *)&peer_sin , &len );
                    if ( thread[i].socket == -1){
                         error_message(ERROR_SOCKET_ACCEPT);
                    }
                    if ( i < cfg->max_connection){                                 //max_connectionを超えていない
                         status_send(thread[i].socket, SERVER_OK, HEADER_LENGTH);  //ヘッダを送る:000(エラーなし)
                         printf("接続しました:%s\n",inet_ntoa(peer_sin.sin_addr));   //クライアントのIPアドレスを表示
                         thread[i].state = 0;                                      //スレッドのステータスを接続待ちに変える
                         pthread_create(&thread_id[i], NULL, (void *)connect_thread, &thread[i]); //スレッドを作る
                         pthread_detach(thread_id[i]);                                            //スレッドの終了は待たない
                         break;
                    } else {                                                       //max_connectionを超えている
                         status_send(thread[i].socket, SERVER_SERVICE_UNAVAILABLE, HEADER_LENGTH);//ヘッダを送る:503(アクセス過多)
                         printf("接続過多により接続を拒否します:%s\n",inet_ntoa(peer_sin.sin_addr));
                         err = close(thread[i].socket);                            //通信用ソケットを破棄
                         if (err == -1){
                              error_message(ERROR_SOCKET_CLOSE);
                         }
                    }
               }//if
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
          thread[i].state = -1;                     //初期化
     }
     memset(&hints,0,sizeof(hints));
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE;
     hints.ai_protocol = 0;
     hints.ai_canonname = NULL;
     hints.ai_addr = NULL;
     hints.ai_next = NULL;
     
     err = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
     if (err != 0){
          printf("getaddrinfo FAILED\n");
          exit(0);
     }
     rp = result;                                   //getaddrinfoで受け取ったアドレス構造体の配列
     for (rp = result; rp != NULL; rp = rp->ai_next) { //構造体ごとにバインドを試す
          *listening_socket = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
          if (*listening_socket == -1){              //ソケットが作れなかったらスルー
               printf("a\n");
               continue;
          }
          setsockopt(*listening_socket, SOL_SOCKET, SO_REUSEADDR, &sock_optival, sizeof(sock_optival) );
          if (bind(*listening_socket, rp->ai_addr, rp->ai_addrlen) == 0){
            break;                                  //バインド成功
          }
          close(*listening_socket);                  //バインドに失敗したのでソケットを閉じる
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
//接続数を表示
void connection_number(threadinfo_t *thread){
     int state_check;
     //thread[i].state(1:接続中 0:接続待機 -1:ヒマ)
     state_check = 0;
     for (int i = 0 ; i < cfg->max_connection ; i++){
          if (thread[i].state != -1 ){
               state_check ++;
          }
//   printf("%d.",thread[i].state);
     }
     printf("接続数:(%d/%d)\n",state_check,cfg->max_connection);
}

//子スレッドでの処理
void connect_thread(threadinfo_t *thread){
     int err;
     
     while (1){
          thread->state = 1;                                //スレッドのステータスを接続中に変える
          server_receive_transmission(thread->socket);      //データのやりとり

          err = close(thread->socket);
          if (err == -1){
               error_message(ERROR_SOCKET_CLOSE);
          }
          thread->state = -1;                               //スレッドのステータスを解放済みに変える
          printf("接続が切れました。\n");
          break;
     }
     connection_number(thread);
}

//送受信
int server_receive_transmission(int socketid){
     char filedata[FILESIZEMAX];
     char filename[256];
     
     memset(filedata,'\0',FILESIZEMAX);
     memset(filename,'\0',256);
     
     error_message( receive_filename(socketid , filename) );
     error_message( transmission_filedata(socketid , filename, filedata) );
     
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
     printf("受信しました。>>%s\n",buf);
     strcpy(filename,"root/");
     strncat(filename,buf,250);
     return 0;
}

//ファイルの読み込み・送信
//filenameでファイル名を受け取り、filedataで返す
int transmission_filedata(int socketid, char *filename, char *filedata){
     char buf[256];                                 //ファイル読込用バッファ
     char filesizebuf[FILESIZE_LENGTH+1];
     int index = 0;
     int filesize;
     memset(buf,'\0',256);
     memset(filesizebuf,'\0',FILESIZE_LENGTH+1);
     FILE *fp;
     
     
     fp = fopen(filename,"r");
     if ( fp == NULL ){                         //ファイルなし
          printf("FILE NOT FOUND\n");
          status_send(socketid, STATUS_NOTFOUND, STATUS_LENGTH);   //404:ファイルがなかった場合のヘッダ

     } else {                                   //ファイルあり
          status_send(socketid , STATUS_OK, STATUS_LENGTH);        //200:ファイルがあった場合のヘッダ
          
          fseek(fp, 0, SEEK_END);                                  //ポインタを最後まで進めて
          filesize = ftell(fp);                                    //ファイルサイズ取得(最終ポインタの位置)
          fseek(fp, 0, SEEK_SET);                                  //ポインタを戻す
//        printf("filesize:%d\n",filesize);
          sprintf(filesizebuf,"%9d",filesize);                    //10桁で0詰めする                       //FILESIZELENGTHと同等
          fread(filedata, sizeof(char), filesize, fp);             //filedataにfilesize分ファイルを読み込む
          
          write(socketid, filesizebuf,FILESIZE_LENGTH);            //桁詰めした文字数を送信する
          do{
               index += write(socketid, (filedata + index) , 1);   //ファイル内容を送信
          } while (index != filesize);
          printf("送信しました。\n");
     }//else
     return NO_ERROR;
}

//ステータスの送信
void status_send(int socketid, char *status, int length){
     int index = 0;
     int writesize = 0;
 //    printf("status:%s\n", status);
     do{
          if ( (writesize = write(socketid, status+index , 1)) > 0){
               index++;
          }
     } while (index != length);
     //送信ステータスの表示
 /*    printf("HEADER:");
     for (int i = 0;i<length;i++){
          if (status[i] == 0){printf("!");}
          else{printf("%c",status[i]);}
     }printf("\n");*/
}