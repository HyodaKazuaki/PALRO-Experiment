//////////////////////////////////////////////////////////////////
// EchoServer　サーバプログラム                   	   		  	//
// コンパイル方法： gcc -o EchoServer EchoServer.cpp -lstdc++		//
// 実行方法： ./EchoServer                               	  	//
// 動作内容： ネットワークから文字列を受け取り、画面表示			//
// 終了方法：　Ctrl+c                        	         	 	//
//////////////////////////////////////////////////////////////////

#include <stdio.h>		// fprintf, perrorの定義
#include <stdlib.h>		// exitの定義
#include <string.h>		// memsetの定義
#include <signal.h>		// signal, SIGINTの定義
#include <netdb.h>		// sockaddr_in, accept, AF_INET, SOCK_STREAM, socket, INADDR_ANY, htons, bind, listenの定義
#include <unistd.h>		// close, read, writeの定義

#define BUFSIZE 1024						// バッファサイズ
#define PORT 54321							// ポート番号

int sock = -1;
int fd = -1;

//////////////////////////////////////////////////////////////////
// Ctrl+cを押したときの処理
//////////////////////////////////////////////////////////////////
void sigint_handler( int sig )
{
	fprintf( stderr, "sigint_hander()\n" );

	if( sock >= 0 )	(void)close( sock );
	if( fd >= 0 )	close( fd );
	exit( 1 );
}

///////////////////////////////////////////////////////////////////////////
// クライアントからの接続があったときの処理
///////////////////////////////////////////////////////////////////////////
void accept_and_echo()
{
	struct sockaddr_in caddr;				// ソケットアドレス構造体
	int ret;
	socklen_t len;
	char buf[BUFSIZE];

	len = sizeof( caddr );
	fd = accept( sock, (struct sockaddr *)&caddr, &len );
	if( fd < 0 ){
		perror( "accept" );
		close( fd );
		exit( 1 );
	}
	
	printf( "accepted\n" );
	
	int rsize;
	while( ( rsize = read( fd, buf, BUFSIZE ) ) > 0 ){
		write( fd, buf, rsize );
		printf( "%s\n", buf );
	}
	
	close( fd );
	printf( "connection finished\n" );
}

///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
int main( void )
{
	int i;
	struct sockaddr_in saddr;

	signal( SIGINT, sigint_handler );							// ctrl+C入力の処理関数

	if(( sock = socket( AF_INET, SOCK_STREAM, 0 )) < 0 ){		// ソケットの作成
		perror( "sock" );
		exit( 1 );
	}

	memset( (void*)&saddr, 0, sizeof( saddr ) );				// ソケットをメモリ領域に確保
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(PORT);

	if( bind( sock, (struct sockaddr *)&saddr, sizeof( saddr ) ) < 0 ){
		perror( "bind" );
		close( sock );
		exit( 1 );
	}

	if( listen( sock, 1 ) < 0 ){
		perror( "listen" );
		close( sock );
		exit( 1 );
	}

	while( 1 ){
		accept_and_echo();
	}

	return 0;
}

