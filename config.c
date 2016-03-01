#include "header.h"

/*
config.cで行う機能

設定ファイルの読み込み、判別
*/

//コンフィグを読み込む
int config_load(){
    char configfile[256];
    FILE *fp;
    char *newline;
    memset(cfg,'\0',sizeof(config_t));

    if ( (fp = fopen(CONFIG_FILE,"r")) == NULL ){   //read-mode
        fclose(fp);
        return CONFIG_NOTFOUND;                     //コンフィグがない
    }
    while( fgets(configfile, 255, fp) != NULL ){
        if ( (newline = memchr(configfile,'\n',strlen(configfile)) ) != NULL ){
            *newline = '\0';
        }
        config_param(configfile, CONFIG_SYMBOL_HOST);
        config_param(configfile, CONFIG_SYMBOL_MAXC);
    }
    if ( (cfg->max_connection < CONNECT_MAXMIN) || (cfg->max_connection > CONNECT_MAXMAX) ){
        printf("最大接続数は1から1000の範囲で設定してください。\n最大接続数:%dに再設定しました。\n", CONNECT_MAX);
        cfg->max_connection = CONNECT_MAX;      //最大接続数にデフォルトの値を設定
    }   
    fclose(fp);
    return NO_ERROR;
}

//configfileのパラメータを切り出し、cfg->symbolnameに代入する
int config_param(char *configfile, char *symbolname){
    char param[256];
    int i = 0;
    int j = 0;
    memset(param,'\0',256);
    
    if ( strncmp(configfile , symbolname, strlen(symbolname)) != 0 ){//文頭がsymbolnameでない場合は失敗
        return 1;
    }
    while ( configfile[i++] != '=') {                  // = が来るまで送る 1バイト目が=でないことは前条件式で保障されている
        if ( i > strlen(configfile)){
            return 1;                                     // = がなければ失敗
        }
    }
    while ( configfile[i] == ' ') {
        i++;                                           //スベースが入っていればその分文字を進める
    }
    while ( !( (configfile[i] == '\n') || (configfile[i] == '\0')) ){
        param[j] = configfile[i];                      //\0でも\nでもない場合にコピー
        i++;
        j++;
    }
    param[j] = '\0';
    if ( !strncmp(symbolname, CONFIG_SYMBOL_HOST, strlen(symbolname)) ){//ホスト名
        memset(cfg->host,'\0',120);
        strncpy(cfg->host,param,119);                      //コンフィグ構造体にコピーする
    } else if ( !strncmp(symbolname, CONFIG_SYMBOL_MAXC, strlen(symbolname)) ){//最大接続数
        cfg->max_connection = atoi(param);
    }

    return 0;
}

//クライアントモードからはnew_config() -> config_load()
//コンフィグファイルを上書きする(新規作成も含む)
int new_config(){
    char input[256];
    char host[261];
    char *newline;
    FILE *fp;
    
    memset(input,'\0',256);
    memset(host,'\0',256);
    
//  printf("接続するホストを入力してください。\n>>");
//  この関数を呼び出す側でメッセージを出力しておく。
    fflush(stdin);
    fgets(input , 255 , stdin);
    
    if (input[0] == '\n') {
        printf("ファイルは変更されませんでした。");
        return 1;
    } else {
        newline = memchr(input , '\n', 255);        //fileの終端にある改行コードを検出する
        *newline = '\0';                            //'\0'に置き換える
        fp = fopen(CONFIG_FILE,"w");
        sprintf(host,"host=%s\nmax_connection=%d",input,cfg->max_connection);
        fputs(host,fp);
        printf("接続先が%sに変更されました。",input);
        fclose(fp);
    }
    return 0;
}