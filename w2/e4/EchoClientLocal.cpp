//////////////////////////////////////////////////////////
// EchoClientLocal クライアントプログラム					//
// コンパイル方法： makefileを使用						//
// 実行方法： ./EchoClientLocal							//
// 動作内容： サーバと接続するだけ							//
// 終了方法： サーバと接続後自動終了						//
//////////////////////////////////////////////////////////

#include <netdb.h>						// gethostbynameの定義
#ifdef __MACH__
#include <opencv/highgui.h>					// CV_LOAD_IMAGE_ANYDEPTH, CV_LOAD_IMAGE_ANY_COLOR, cvLoadImage, cvSaveImage, CV_WINDOW_AUTOSIZE, cvNamedWindow, cvShowImage, cvWaitKey, cvDestroyWindowの定義
#endif
#ifndef __MACH__
#include <highgui.h>					// CV_LOAD_IMAGE_ANYDEPTH, CV_LOAD_IMAGE_ANY_COLOR, cvLoadImage, cvSaveImage, CV_WINDOW_AUTOSIZE, cvNamedWindow, cvShowImage, cvWaitKey, cvDestroyWindowの定義
#endif
#include <stdio.h>
#include <fcntl.h>						// O_RDONLY　を使用するため
#include <unistd.h>						// read, close　など
#include <netdb.h>						// gethostbynameの定義

#define BUFF_SIZE 1024					// バッファサイズ
#define PORT 54321						// ポート番号
#define HOST_NAME "localhost"			// サーバ名
	
///////////////////////////////////////////////////////////////////////////
// ネットワーク開始処理
// 引数：なし
// 返り値：ソケットのディスクリプター
///////////////////////////////////////////////////////////////////////////
int OpenNetwork( )
{
	int sock;									// ここから一般的なソケット処理
	if(( sock = socket( AF_INET, SOCK_STREAM, 0 )) < 0 ){
		perror( "sock" );
		return -1;
	}

	struct hostent *hp = gethostbyname( HOST_NAME );
	if( hp == NULL ){
		perror( "gethostbyname" );
		return -1;
	}

	struct sockaddr_in addr;
	memset( (void*)&addr, 0, sizeof( addr) );
	memcpy( &addr.sin_addr, hp->h_addr, hp->h_length );
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);

	if( connect( sock, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ){
		perror( "connect" );
		return -1;
	}
	return sock;
}

///////////////////////////////////////////////////////////////////////////
// ネットワーク終了処理
// 引数：ソケットディスクリプター
// 返り値：正常終了0、異常終了-1
///////////////////////////////////////////////////////////////////////////
int CloseNetwork( int sock )
{
	close( sock );

	return 0;
}
		
///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
int main( void )
{
	int sock = -1;
	char buf[BUFF_SIZE];							// データ転送用配列
	// ネットワーク接続
	sock = OpenNetwork();						// ソケットのハンドル取得
	if( sock < 0 ){
		return -1;
	}
	printf( "Connected to Server.\n");
	
	/* ここに送信処理を挿入 */
	while(fgets(buf, BUFF_SIZE, stdin) != NULL && buf[0] != '\n'){
		write(sock, buf, BUFF_SIZE);
		read(sock, buf, BUFF_SIZE);
		printf("server: %s", buf);
	}
	write(sock, buf, BUFF_SIZE);

	CloseNetwork( sock );
	printf("Closed connection.\n");
}

