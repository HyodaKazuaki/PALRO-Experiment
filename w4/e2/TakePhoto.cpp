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
#include <netdb.h>

#define BMP_FILE_NAME "tmp.bmp"			// 一時保存ファイル名
#define BUFF_SIZE 1024					// バッファサイズ
#define PORT 54321						// ポート番号
#define HOST_NAME "192.168.11.57"			// サーバ名

class CTakePhoto : public PAPI::CTransientApplication
{
private:
	const Sapie::CEventInfo *pEventInfo;
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

	void detectObject(IplImage* img, double *th_h, double *th_s, double *th_v) {
		int x, y;
		int pos;
		double hsv[3], rgb[3];
		for(y = 0; y < img->height; y++){
			for(x = 0; x < img->width; x++){
				pos = img->widthStep * y + x * 3;
				rgb[0] = (uchar)img->imageData[pos + 2 ] / 255.0;        // R読み込み
				rgb[1] = (uchar)img->imageData[pos + 1 ] / 255.0;        // G読み込み
				rgb[2] = (uchar)img->imageData[pos + 0 ] / 255.0;        // B読み込み
				RGBtoHSV( rgb, hsv );
				if(
					th_h[0] < hsv[0] and hsv[0] < th_h[1] and
					th_s[0] < hsv[1] and hsv[1] < th_s[1] and
					th_v[0] < hsv[2] and hsv[2] < th_v[1]
				){
					hsv[1] = 0.0;
					hsv[2] = 1.0;
				}
				HSVtoRGB(hsv, rgb);
				img->imageData[pos + 0] = cvRound(rgb[2] * 255.0);
				img->imageData[pos + 1] = cvRound(rgb[1] * 255.0);
				img->imageData[pos + 2] = cvRound(rgb[0] * 255.0);
			}
		}
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

	int SendFile(int sock, char* filename, int buff_size){
		int ret = 0;
		char buf[buff_size];							// データ転送用配列

		int bfd = open( filename, O_RDONLY );		// ビットマップファイルをバイナリ形式でオープン
		if( bfd < 0 ){
			mySpeak("写真を開くことができませんでした");
			return -1;
		}

		/* ここに送信処理を挿入 */
		do{
			ret = read(bfd, buf, buff_size);
			if(ret < 0){
				mySpeak("写真を読み込めませんでした");
				return -1;
			}
			if(write(sock, buf, ret) < 0){
				mySpeak("写真を送信できませんでした");
				return -1;
			}
			bmpSize += ret;
		}while(ret != 0);
		close( bfd );									// ビットマップファイルのクローズ
		return 0;
	}

///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
	void OnProcessMain(Sapie::CControllerBase* pSender)
	{
		double average;
		char buf[BUFF_SIZE], recv[BUFF_SIZE];
		char str[300];
		double th_h[2] = {280.0, 351.0};
		double th_s[2] = {0.55, 1.0};
		double th_v[2] = {0.0, 1.0};
		// ネットワーク接続
		sprintf(str, "%sに接続します", HOST_NAME);
		mySpeak(str);
		int sock = OpenNetwork();						// ソケットのハンドル取得
		if(sock < 0){
			sprintf(str, "%sの接続に失敗しました", HOST_NAME);
			mySpeak(str);
			return;
		}
		sprintf(str, "%sに接続しました", HOST_NAME);
		mySpeak(str);
		mySpeak( "ボールを探してみます" );
		while(1){
			IplImage* img = NULL;
			TakePic( img );
			average = CalcAveBrightness(img);
			detectObject(img, th_h, th_s, th_v);
			cvSaveImage( BMP_FILE_NAME, img );					// 撮った写真をBMPファイルとして保存
			cvReleaseImage( &img );
			if(SendFile(sock, BMP_FILE_NAME, BUFF_SIZE) != 0) break;
			if(average < 0.3) break;
		}
		CloseNetwork( sock );
		sprintf(str, "%sとの接続を解除しました", HOST_NAME);
		mySpeak(str);
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CTakePhoto());
}