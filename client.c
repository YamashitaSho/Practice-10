#include "header.h"

/*
client.cでやること
メニューの表示
1.ファイル取得
サーバプログラムに接続し、ファイル名を送信する。
サーバプログラムから返信されたファイルの内容を受け取る。
2.設定
設定ファイルを読み込む。なければ作成する。
9.終了
*/

int client_main(int debugmode){
    int mode = debugmode;
    int err;
    
    if (debugmode == 0){
        mode = get_menu();                      //モード選択 デバッグモード時は最初の周は省略する
    }
    while(1){
        if ( mode == CLIENT_GET ){              //ファイル取得
            if ( cfg->host[0] == '\0' ){
                printf("接続先が登録されていません。\nEnterキーでメニューに戻ります。\n");
                fflush(stdin);
                getc(stdin);
                mode = get_menu();
                continue;
            }
            err = client_receive_transmission(cfg);
            if ( err == ERROR_CONNECT ){
                printf("サーバに接続できませんでした。\n接続先の受信プログラムが起動していない可能性があります。\nEnterキーでもう一度接続を試みます。\n");
                fflush(stdin);
                getc(stdin);
                mode = get_menu();
                continue;
            }
            if ( err == 503 ){
                error_message(503);
            }
        } else if ( mode == CLIENT_CONFIG ){    //設定
            if ( cfg->host[0] == '\0' ){            //接続先がないので入力させる
                printf("接続先を入力し、Enterキーを押してください。\n>>");
                new_config();
                config_load();
            } else {                                //接続先を表示し、入力を求める。
                printf("現在設定されている接続先は%sです。\n上書きしない場合は、Enterキーのみを入力してください。\n上書きする場合は接続先を入力し、Enterキーを入力してください。\n>>",cfg->host);
                new_config();
                config_load();
            }
        } else if ( mode == CLIENT_EXIT ){      //終了
            exit(0);
        }
        mode = get_menu();
    }
    return NO_ERROR;
}

char get_menu(){
    char input[50];
    char mode;
    
    printf("\n1.ファイル取得\n2.設定\n9.終了\n\nメニュー番号を入力してください:");
    fflush(stdin);
    fgets(input,50,stdin);
    mode = input[0];
    
    return mode;
}

void input_a_line(char *inputline){
    char input[256];
    char *newline;                                  //改行検出用
    
    printf("\n読み込むファイル名を入力してください。>>");
    fflush(stdin);
    fgets(input , 255 , stdin);
    
    if (input[0] != '\n') {
        newline = memchr(input , '\n', 255);        //fileの終端にある改行コードを検出する
        *newline = '\0';                            //'\0'に置き換える
    }
    strncpy(inputline , input , 255);
}

//接続、受送信
int client_receive_transmission(config_t *cfg){
    struct sockaddr_in client_addr;                 //ネットワーク設定
    struct in_addr servhost;                        //サーバーのIP格納
    int port = atoi(DEFAULT_PORT);                        //ポート番号
    int ret;                                        //返り値の一時保存
    int connecting_socket = 0;                      //接続時のソケットディスクリプタ
    char filename[256];                             //取得リクエストを送るファイル名
    char receive_data[FILESIZEMAX];                    //受け取るデータ
    int filesize;                                   //受け取るデータのサイズ
    FILE *socketfp;                                 //接続ストリームを受け取るファイルポインタ

    memset(receive_data,'\0',FILESIZEMAX);
    
    //ソケットを作成
    if ( (connecting_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error_message(ERROR_SOCKET);
    }
    
    printf("接続します:%s\n", cfg->host );
    
    //ホスト名の解決
    if ( address_resolution(&servhost) == 0){
        return ERROR_HOST_UNKNOWN;
    }
    //アドレスファミリー・ポート番号・IPアドレス設定       設定ファイルを適用する部分
    memset(&client_addr, '\0' ,sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr = servhost;
    
    ret = connect( connecting_socket, (struct sockaddr *)&client_addr, sizeof(client_addr) );
    if ( ret < 0 ){
        return ERROR_CONNECT;
    }
    socketfp = fdopen(connecting_socket,"rb");            //バイナリモード読み取り専用で受信データを開く
    ret = receive_status(socketfp, HEADER_LENGTH);
    if ( ret == 503 ){
        close(connecting_socket); 
        return 503;
    }
    
    printf("サーバーに接続しました。\n");
    transmission_filename(connecting_socket, filename);
    ret = receive_status(socketfp, STATUS_LENGTH);
    if (ret == 200){
        filesize = receive_status(socketfp, FILESIZE_LENGTH);
        receive_filedata(socketfp, receive_data, filesize, filename);
    } else if (ret == 404){
        printf("FILE NOT FOUND\n");
    }
    
    close( connecting_socket );

    return NO_ERROR;

}
//ホスト名を解決
int address_resolution(struct in_addr *servhost){
    struct addrinfo hint;                           //getaddrinfoを使うためにaddrinfo型の構造体を宣言
    struct addrinfo *result;                        //getaddrinfoの結果を受け取る構造体
    
    memset( &hint , 0 , sizeof(struct addrinfo) );
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = 0;
    hint.ai_protocol = 0;
    
    if ( getaddrinfo(cfg->host,NULL,&hint,&result) != 0 ) {
        return 0;
    }
    servhost->s_addr=((struct sockaddr_in *)(result->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(result);
    return 1;
}
//ステータス受信
int receive_status(FILE *fp, int length){
    int index = 0;
    int readsize = 0;
    char status[length];
    char bufc;
    
    memset(status,'\0',length);
    do { 
        if ( (readsize = fread(&bufc, sizeof(char) , 1, fp)) > 0){
            status[index] = bufc;
            index ++;
        }
    } while (index != length);
    
    //受信したヘッダの表示
/*    printf("STATUS:");
    for (int i = 0 ; i < length ; i++){
        if (status[i] == 0){printf("!");}
        else{printf("%c",status[i]);}
    }
    printf("\n");
    printf("atoi(status):%d\n",atoi(status));*/
    return atoi(status);
}

//ファイル名送信
int transmission_filename(int socketid, char *filename){
    int buf_len = 0;
    
    do{
        memset(filename,'\0',256);
        input_a_line(filename);
    } while ( filename[0] == '\n' );
    do{
        buf_len += write( socketid , (filename+buf_len) , 255 );
    } while ( filename[buf_len] != '\0' );
    printf("ファイル名:%sを送信しました。\n",filename);
    return NO_ERROR;
}

//ファイル受信
int receive_filedata(FILE *receive_fp, char *receive_data, int filesize, char *filename){
    char bufc;                                                  //ソケット受信用バッファ
    int size_t;                                                 //受信したかどうかのバッファ
    int index = 0;                                              //受信した合計量
    
    int i = 1;                                                  //ファイル検索における通し番号管理に使用(1から始める)
    char *extensionp = memchr(filename, '.', strlen(filename)); //拡張子開始位置のポインタ
    const char dir[12] = SAVE_DIRECTORY;
    char extension[256];                                        //拡張子
    char filename_name[256];                                    //ファイル名の拡張子を除いた部分
    char filename_dir[512];                                     //ディレクトリ＋ファイル名＋(被り防止番号)＋拡張子
    FILE *save_fp;
    
    //変数の初期化
    memset(filename_name, '\0', 256);
    memset(extension, '\0', 256);
    memset(filename_dir,'\0',512);
    
    memcpy(extension, extensionp, strlen(filename)-(extensionp-filename));
    memcpy(filename_name, filename, extensionp-filename);
    
    //ファイルの受信
    do{
        if ( (size_t = fread(&bufc, 1, 1, receive_fp)) > 0 ){
           receive_data[index] = bufc;
           index += size_t;
        }
    } while (index != filesize);
    printf("%dバイト受信しました。\n", filesize);
    
    dir_make(SAVE_DIRECTORY);
    //保存ファイル名の決定
    sprintf(filename_dir,"%s/%s",dir,filename);
    save_fp = fopen(filename_dir, "r+");
    if ( save_fp != NULL ){                                     //すでにfilename_dirが存在している
        do{
            fclose(save_fp);
            sprintf(filename_dir, "%s/%s(%d)%s", dir, filename_name, i, extension);   //新しいファイル名(失敗回数)
            save_fp = fopen(filename_dir, "r+");
            i++;
        } while ( save_fp != NULL);
        printf("重複ファイルがありました\n");
    }
    fclose(save_fp);
    
    save_fp=fopen(filename_dir,"w");
    fwrite(receive_data, filesize, 1, save_fp);
    printf("%sに保存しました。", filename_dir);
    fclose(save_fp);
    return NO_ERROR;
}

//directoryフォルダの存在を判定 なければ作る
void dir_make(char *directory){
    DIR *dir;
    char path[512] = "./";
    strncat(path,directory,strlen(directory));
    
    if ((dir = opendir(path)) != 0){
        closedir(dir);
    } else {
        mkdir(path,0777);
        chmod(path,0777);
    }
}