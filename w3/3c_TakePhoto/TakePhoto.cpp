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
#include <string>
#include <netdb>

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

///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
	void OnProcessMain(Sapie::CControllerBase* pSender)
	{
		mySpeak( "写真を撮ります" );
		IplImage* img = NULL;
		TakePic( img );
		mySpeak("写真を撮ったので保存します");
		cvSaveImage( BMP_FILE_NAME, img );					// 撮った写真をBMPファイルとして保存
		cvReleaseImage( &img );
		std::string sending = "今から" + HOST_NAME + "に写真を送ります";
		mySpeak(sending.c_str())

		int ret = 0;
		// ネットワーク接続
		int sock = OpenNetwork();						// ソケットのハンドル取得
		mySpeak("ネットワークに接続しました");

		int bfd = open( BMP_FILE_NAME, O_RDONLY );		// ビットマップファイルをバイナリ形式でオープン
		if( bfd < 0 ){
			return 1;
		}

		char buf[BUFF_SIZE];							// データ転送用配列
		long bmpSize = 0;

		/* ここに送信処理を挿入 */
		do{
			ret = read(bfd, buf, BUFF_SIZE);
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
		mySpeak("写真を送信しました");
		close( bfd );									// ビットマップファイルのクローズ

		CloseNetwork( sock );
		mySpeak("ネットワークとの接続を解除しました");
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CTakePhoto());
}
