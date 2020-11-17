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
#define ECHO_PORT 54321						// ポート番号
#define IMGPORT 54322
#define HOST_NAME "192.168.11.57"			// サーバ名

class CTakePhoto : public PAPI::CTransientApplication
{
private:
	const Sapie::CEventInfo *pEventInfo;
	///////////////////////////////////////////////////////////////////////////
	// HSV表色系からHSV表色系への変換
	// 引数：	HSV(H:0~360, S,V:0~1)の入ったdouble配列
	//			RGB(R, G, B:0~1)を格納するdouble配列
	// 返り値：なし
	///////////////////////////////////////////////////////////////////////////
	void HSVtoRGB( double pre[3], double post[3] )
	{
		double h = pre[0];
		double s = pre[1];
		double v = pre[2];
		int Hi = (int)(h / 60) % 6;
		double f = h / 60.0 - Hi;
		double p = v * (1.0 - s);
		double q = v * (1 - f * s);
		double t = v * (1.0 - (1.0 - f) * s);
		switch (Hi)
		{
		case 0:
			post[0] = v;
			post[1] = t;
			post[2] = p;
			break;
		case 1:
			post[0] = q;
			post[1] = v;
			post[2] = p;
			break;
		case 2:
			post[0] = p;
			post[1] = v;
			post[2] = t;
			break;
		case 3:
			post[0] = p;
			post[1] = q;
			post[2] = v;
			break;
		case 4:
			post[0] = t;
			post[1] = p;
			post[2] = v;
			break;
		case 5:
			post[0] = v;
			post[1] = p;
			post[2] = q;
			break;
		default:
			break;
		}
	}

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
		if (h < 0)
			h += 360.0;
		s = (max - min) / max;
		v = max;
		post[0] = h;
		post[1] = s;
		post[2] = v;
	}
	long MoveNeck(int yaw, int pitch, int msec)
	{
		if(yaw < -60 || yaw > 60 || pitch < -45 || pitch > 17 || msec < 10)
			return -1;
		PAPI::CCommParams cParam;

		cParam.Set(PAPI::PARAM_NECK_YAW, yaw);
		cParam.Set(PAPI::PARAM_NECK_PITCH, pitch);
		cParam.Set(PAPI::PARAM_NECK_EXEC_TIME, msec);
		long lReqno = cComm.Send(PAPI::COMTYP_MOVE_NECK_FREELY, NULL, cParam);
		long lResult = evDispatcher.Sync(lReqno);
		if(lResult != 0)
			SAPIE_LOG_ERR(0, "MoveNeckError: %ld", lResult);
		return lResult;
	}
	///////////////////////////////////////////////////////////////////////////
	// ネットワーク開始処理
	// 引数：なし
	// 返り値：ソケットのディスクリプター
	///////////////////////////////////////////////////////////////////////////
	int OpenNetwork(int port)
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
		addr.sin_port = htons(port);

		if( connect( sock, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ){
			perror( "connect" );
			return -1;
		}
		return sock;
	}

	///////////////////////////////////////////////////////////////////////////
	// HSV表色系において色相がlow~highの間で、彩度がlows以上の画素数を数え、色を白くし、重心に丸を打つ
	// 引数：	img: 	解析するBMP画像ポインタ
	//      posOfGrav: 重心の座標
	//     saturation: 彩度
	//     brightness: 明度
	// 返り値：該当する画素数
	///////////////////////////////////////////////////////////////////////////
	long DrawAndCalcCenterOfGravity(IplImage* img, int* posOfGrav, double saturation, double brightness)
	{
		long res = 0;
		double rgb[3], hsv[3];
		
		CvPoint grav;
		grav.x = 0;	grav.y = 0;
		
		for( int y = 0; y < img->height; y++ ){
			for( int x = 0; x < img->width; x++ ){
				int pos = img->widthStep * y + x * 3;
				rgb[0] = (uchar)img->imageData[pos + 2 ] / 255.0;        // R読み込み
				rgb[1] = (uchar)img->imageData[pos + 1 ] / 255.0;        // G読み込み
				rgb[2] = (uchar)img->imageData[pos + 0 ] / 255.0;        // B読み込み
				
				RGBtoHSV( rgb, hsv );
				
				/* ここで重心を計算する */
				if(hsv[1] == saturation && hsv[2] == brightness) {
					grav.x += x;
					grav.y += y;
					res += 1;
				}
			}
		}
		if( res != 0 ){
			grav.x /= res;
			grav.y /= res;
			cvCircle( img, grav, 3, CV_RGB( 255, 0, 0 ) );
			posOfGrav[0] = grav.x;
			posOfGrav[1] = grav.y;
		}
		
		return res;
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
		long count;
		int posOfGrav[2] = {0, 0};
		int prePosOfGrav[2] = {0, 0};
		int yaw, pitch;
		int move_time = 5;
		char buf[BUFF_SIZE], recv[BUFF_SIZE];
		char str[300];
		double th_h[2] = {280.0, 351.0};
		double th_s[2] = {0.55, 1.0};
		double th_v[2] = {0.0, 1.0};
		// ネットワーク接続
		sprintf(str, "%sに接続します", HOST_NAME);
		mySpeak(str);
		int echo_sock = OpenNetwork(ECHO_PORT);						// ソケットのハンドル取得
		int img_sock = OpenNetwork(IMGPORT);						// ソケットのハンドル取得
		if(echo_sock < 0 or img_sock < 0){
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
			if(average < 0.3) break;
			detectObject(img, th_h, th_s, th_v);
			count = DrawAndCalcCenterOfGravity(img, posOfGrav, 0.0, 1.0);
			if(count > 0) {
				// 追跡
				yaw = posOfGrav[0] - (int)(img->width / 2.0);
				pitch = posOfGrav[1] - (int)(img->height / 2.0);
				MoveNeck(yaw, pitch, move_time);
			}
			cvSaveImage( BMP_FILE_NAME, img );					// 撮った写真をBMPファイルとして保存
			cvReleaseImage( &img );
			if(SendFile(sock, BMP_FILE_NAME, BUFF_SIZE) != 0) break;
			sprintf(buf, "yaw: %d, pitch: %d\n", yaw, pitch);
			write(sock, buf, sizeof(buf));
			read(sock, recv, BUFF_SIZE);
			if(strcmp(buf, recv) != 0) {
				mySpeak("サーバとの通信がおかしいようです");
				break;
			}
		}
		CloseNetwork( echo_sock );
		CloseNetwork( img_sock );
		sprintf(str, "%sとの接続を解除しました", HOST_NAME);
		mySpeak(str);
		mySpeak("Thanks for getting in touch with me.")
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CTakePhoto());
}
