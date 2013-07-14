fileServer
==========

fileServerもどき  
"Linux ネットワークプログラミングバイブル"のサンプルコードを改変  

# server.c
コマンドライン引数で与えたポート番号で待機  
telnetで接続すると基本的にechoServerとして動作する  
GET "filename"が入力されたときだけfileServerとしてファイルを返す  
fileNameSize: ファイル名のバイト数(パスを含む)byte  
fileName: ファイル名  
fileSize: ファイルのバイト数byte  
(DATA)  