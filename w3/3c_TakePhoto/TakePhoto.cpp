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

#define BMP_FILE_NAME "tmp.bmp"			// 一時保存ファイル名

class CTakePhoto : public PAPI::CTransientApplication
{
private:
	const Sapie::CEventInfo *pEventInfo;

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
		cvSaveImage( BMP_FILE_NAME, img );					// 撮った写真をBMPファイルとして保存
		cvReleaseImage( &img );
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CTakePhoto());
}
