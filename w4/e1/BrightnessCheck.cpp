//////////////////////////////////////////////////////////
// TakePhoto クライアント側プログラム					//
// コンパイル方法： makefileを使用						//
// 実行方法： PALROにインストール後、「カメラやって」で起動	//
// 動作内容： 写真撮影し、ファイルをサーバに転送			//
// 終了方法： サーバにファイルを転送後自動終了				//
//////////////////////////////////////////////////////////

#include "TransientApplication.h"		// PALROの非常駐型アプリケーション関係
#include "LogCtrl.h"					// PALROのログ関係
#include "CameraController.h"			// PALROのカメラコントロール関係
#include <opencv/highgui.h>				// cvSaveImageの定義
#include <fcntl.h>						// O_RDONLY　を使用するため
#include <unistd.h>						// read, close　など
#include <netdb.h>						// gethostbynameの定義

#define BMP_FILE_NAME "tmp.bmp"			// 一時保存ファイル名
#define BUFF_SIZE 1024					// バッファサイズ
#define PORT 54321						// ポート番号
#define HOST_NAME "192.168.11.57"			// サーバ名

class CTakePhoto : public PAPI::CTransientApplication
{
private:
	const Sapie::CEventInfo *pEventInfo;
///////////////////////////////////////////////////////////////////////////
// HSV表色系からHSV表色系への変換
// 引数：	RGB(R, G, B:0~1)を格納するdouble配列
//			HSV(H:0~360, S,V:0~1)の入ったdouble配列
// 返り値：なし
///////////////////////////////////////////////////////////////////////////
void RGBtoHSV( double pre[3], double post[3] )
{
	double h, s, v;
	double r = pre[0];
	double g = pre[1];
	double b = pre[2];

	double max = r;
	double min = r;
	int i;
	for (i = 0; i < 3; i++){
		if(max < pre[i])
			max = pre[i];
		if(min > pre[i])
			min = pre[i];
	}
	if(max == r)
		h = 60.0 * ((g - b) / (max - min));
	else if(max == g)
		h = 60.0 * ((b - r) / (max - min)) + 120.0;
	else
		h = 60.0 * ((r - g) / (max - min)) + 240.0;
	s = (max - min) / max;
	v = max;
	post[0] = h;
	post[1] = s;
	post[2] = v;
}
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
		perror( "socket" );
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
public:
///////////////////////////////////////////////////////////////////////////
// 発話関数
// 引数：PALROに話させたい文字列
// 返り値：0: 正常にイベント待ちが完了、0以外: タイムアウトが発生した、イベント待ちが正常に完了しなかった
///////////////////////////////////////////////////////////////////////////
	long mySpeak( std::string str )
	{
		PAPI::CCommParams cParam;											// 発話パラメータ準備
		cParam.Set( PAPI::PARAM_SENTENCE, str );
		long lReqno = cComm.Send(PAPI::COMTYP_TELL, NULL, cParam);			// 発話開始
		long lResult = evDispatcher.Sync(lReqno);							// 発話完了待ち
		if (lResult != 0) {													// エラー判定
			SAPIE_LOG_ERR( 0, "Error: %ld", lResult );
		}
		return lResult;
	}

///////////////////////////////////////////////////////////////////////////
// 写真撮影処理
// 引数：なし
// 返り値：正常終了0、異常終了-1
///////////////////////////////////////////////////////////////////////////
	void TakePic( IplImage*& img )
	{
		Sapie::CCameraController cCamera;
		cCamera.SetEventDispatcher( &evDispatcher );
		long count = 1;
		long reqno = cCamera.StartCapture( 1, count );
		if( evDispatcher.WaitForEvent( Sapie::EVTYP_IMAGE, reqno, 1000, pEventInfo ) != 0 ){
			mySpeak( "カメライベントが異常終了しました" );
			return;
		}
		
		SAPIE_LOG_ERR( 0, "EventID %ld", pEventInfo->GetId() );
		const Sapie::CReceiveImageEvParam* pParam = (const Sapie::CReceiveImageEvParam*)pEventInfo->GetParam();
		if( img == NULL ){
			img = cvCreateImage( cvGetSize( pParam->pImage ), IPL_DEPTH_8U, 3);
		}
		cvCopy( pParam->pImage, img );
		
		evDispatcher.Release( pEventInfo );
	}

double CalcAveBrightness(IplImage* img) {
	int x, y, pos;
	double hsv[3], rgb[3];
	double ave = 0.0;
	for(y = 0; y < img->height; y++){
		for(x = 0; x < img->width; x++){
			pos = img->widthStep * y + x * 3;
			rgb[0] = (uchar)img->imageData[pos + 2] / 255.0;
			rgb[1] = (uchar)img->imageData[pos + 1] / 255.0;
			rgb[2] = (uchar)img->imageData[pos + 0] / 255.0;
			RGBtoHSV(rgb, hsv);
			ave += hsv[2];
		}
	}
	ave /= img->height * img->width;
	return ave;
}

///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
	void OnProcessMain(Sapie::CControllerBase* pSender)
	{
		double average;
		char buf[BUFF_SIZE], recv[BUFF_SIZE];
		// ネットワーク接続
		char str[300];
		sprintf(str, "%sに接続します", HOST_NAME);
		mySpeak(str);
		int sock = OpenNetwork();						// ソケットのハンドル取得
		if(sock < 0){
			mySpeak("ネットワークの接続に失敗しました");
			return;
		}
		mySpeak("ネットワークに接続しました");
		mySpeak( "今からしばらく写真を撮ります" );
		IplImage* img = NULL;
		while(1){
			TakePic( img );
			average = CalcAveBrightness(img);
			sprintf(buf, "%lf", average);
			write(sock, buf, strlen(buf) + 1);
			read(sock, recv, BUFF_SIZE);
			if(strcmp(buf, recv) != 0) {
				mySpeak("サーバとの通信がおかしいようです");
				break;
			}
			if(average < 0.3)
				break;
		}
		mySpeak("プログラムを終了します");
		cvReleaseImage( &img );

		CloseNetwork( sock );
		mySpeak("ネットワークとの接続を解除しました");
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CTakePhoto());
}
