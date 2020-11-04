//////////////////////////////////////////////
// HelloWorldプログラム						//
// コンパイル方法： makefileを使用			//
// 実行方法： プログラム転送サービス検証用		//
// 動作内容： 「こんにちは」　と言う			//
// 終了方法： 自動終了						//
//////////////////////////////////////////////

#include "TransientApplication.h"		// PALROの非常駐型アプリケーション関係
#include "LogCtrl.h"					// PALROのログ関係

class CHelloWorld : public PAPI::CTransientApplication
{
private:
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
	}
	return lResult;
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
// メイン関数
///////////////////////////////////////////////////////////////////////////
	void OnProcessMain(Sapie::CControllerBase* pSender)
	{
		mySpeak( "こんにちは" );
		MoveNeck(60, -45, 45);
		MoveNeck(-60, 17, 90);
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CHelloWorld());
}
