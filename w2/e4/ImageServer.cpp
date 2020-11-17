//////////////////////////////////////////////////////////////////
// EchoServer　サーバプログラム                   	   		  	//
// コンパイル方法： gcc -o ImageServer ImageServer.cpp -lstdc++	//
// 実行方法： ./ImageServer                               	  	//
// 動作内容： ネットワークから画像データを受け取り、画面表示	//
// 終了方法：　Ctrl+c                        	         	 	//
//////////////////////////////////////////////////////////////////
#include <stdio.h>		// fprintf, perrorの定義
#include <stdlib.h>		// exitの定義
#include <string.h>		// memsetの定義
#include <signal.h>		// signal, SIGINTの定義
#include <netdb.h>		// sockaddr_in, accept, AF_INET, SOCK_STREAM, socket, INADDR_ANY, htons, bind, listenの定義
#include <unistd.h>		// close, read, writeの定義
#include <fcntl.h>		// O_WRONLY, openの定義
#ifdef __MACH__
#include <opencv/highgui.h>					// CV_LOAD_IMAGE_ANYDEPTH, CV_LOAD_IMAGE_ANY_COLOR, cvLoadImage, cvSaveImage, CV_WINDOW_AUTOSIZE, cvNamedWindow, cvShowImage, cvWaitKey, cvDestroyWindowの定義
#endif
#ifndef __MACH__
#include <highgui.h>					// CV_LOAD_IMAGE_ANYDEPTH, CV_LOAD_IMAGE_ANY_COLOR, cvLoadImage, cvSaveImage, CV_WINDOW_AUTOSIZE, cvNamedWindow, cvShowImage, cvWaitKey, cvDestroyWindowの定義
#endif

#define BUFSIZE 1024						// バッファサイズ
#define PORT 54322							// ポート番号
#define BMP_FILE_NAME "COMM"			// 保存ファイル名

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
int accept_and_echo()
{
	struct sockaddr_in caddr;				// ソケットアドレス構造体
	int ret, wsize;
	socklen_t len;
	char buf[BUFSIZE];
	char filename[100];

	len = sizeof( caddr );
	fd = accept( sock, (struct sockaddr *)&caddr, &len );	// サーバソケットのハンドル取得
	if( fd < 0 ){
		perror( "accept" );
		close( fd );
		return 1;
	}
	
	for(int i = 0;; i++){
		sprintf(filename, "./%s_%d.bmp", BMP_FILE_NAME, i);
		int bfd = open( filename, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);				// 画像ファイルのハンドル取得
		if( bfd < 0 ){
			perror( "bmp file open" );
			return 1;
		}
		
		long commlen = 0;
		
		/* ここに受信処理を記述 */
		do{
			ret = read(fd, buf, BUFSIZE);
			if(ret < 0){
				fputs("Failed to read file from network.\n", stderr);
				return 1;
			}
			if(write(bfd, buf, ret) < 0){
				fputs("Failed to write file.\n", stderr);
				return 1;
			}
			commlen += ret;
			printf("\rTotal: %ld B(%d B Recieved)", commlen, ret);
		}while(ret != 0);
		fsync(bfd);
		printf("\n");
		
		close( bfd );
		printf( "%ld B ファイル出力終了\n" , commlen * BUFSIZE / 1024);
		if(buf[0] == '\0') break;
		
		// IplImage* img = cvLoadImage(filename, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR );
		// if(img == 0) return 1;
		// cvNamedWindow("Uploaded Image", CV_WINDOW_AUTOSIZE);
		// cvShowImage("Uploaded Image", img);
		// cvWaitKey(0);
		// cvDestroyWindow("Uploaded Image");
		// cvReleaseImage(&img);
	}

	close( fd );

	return 0;
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

	int tmp= 1;
	if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof( tmp ) ) ){
		printf( "setsockopt() failed!\n" );
	}
	
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

	if(!accept_and_echo()){
	}

	return 0;
}

