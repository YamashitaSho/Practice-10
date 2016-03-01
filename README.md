# Practice-9
##概要
クライアントプログラム（-c オプションで起動）から送信されてきた文字列（ファイル名）を、サーバープログラム（-sオプションで起動）で受信し、rootフォルダのファイルを読み込んでバイナリで内容を返信する。<br>
受信したファイルはlocalフォルダに保存される。(ない場合は自動で作成する。)<br>
コンフィグファイルを用意し、クライアントの接続先とサーバーの最大接続数を設定する。<br>

##コンパイル方法
compile.shを実行する

##実行方法
サーバーモード<br>
tcwebngin -s<br>
クライアントモード<br>
tcwebngin -c<br>