//////////////////////////////////////////////////////////
// EchoClient クライアントプログラム						//
// コンパイル方法： makefileを使用						//
// 実行方法： PALROにインストール後、「文字送り」で起動	//
// 動作内容： 文字列をサーバに転送							//
// 終了方法： 文字列転送後自動終了							//
//////////////////////////////////////////////////////////

#include "TransientApplication.h"		// PALROの非常駐型アプリケーション関係
#include "LogCtrl.h"					// PALROのログ関係
#include "NetSwitchController.h"		// PALROのネットワーク関係
#include <netdb.h>						// gethostbynameの定義

#define PORT 54321						// ポート番号
#define BUFF_SIZE 1024					// バッファサイズ

#define HOST_NAME "192.168.100.101"		// サーバのIPアドレス

class CEchoClient : public PAPI::CTransientApplication
{
private:
	const Sapie::CEventInfo *pEventInfo;
	Sapie::CNetSwitchController cNet;

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
// ネットワーク開始処理
// 引数：なし
// 返り値：ソケットのディスクリプター
///////////////////////////////////////////////////////////////////////////
	int OpenNetwork( )
	{
		cNet.SetEventDispatcher( &evDispatcher );
		long reqno = cNet.EnableNetwork( );
		if( evDispatcher.WaitForEvent( Sapie::EVTYP_UPDATE_NET_STATUS, reqno, 1000, pEventInfo ) != 0 ){
			mySpeak( "ネットワーク接続に失敗しました" );
			return -1;
		}
		evDispatcher.Release( pEventInfo );
		
		Sapie::NetSwitchInfo netInfo;	
		cNet.GetNetworkStatus( netInfo );				// ネットワーク状態の取得
		
		SAPIE_LOG_ERR( 0, "bEnabled %d, nErrorStatus %ld", (int)netInfo.bEnabled, netInfo.nErrorStatus );
													// ここまでPALRO特有の処理

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
		close( sock );									// ここまで一般的なソケット処理
														// ここからPALRO特有の処理
		if( cNet.DisableNetwork() == 0 ){
			mySpeak( "ネットワーク接続完了に失敗しました" );
			return -1;
		}
		return 0;
	}
		
///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
	void OnProcessMain(Sapie::CControllerBase* pSender)
	{
		mySpeak( "ネットワークに接続します" );
		// ネットワーク接続
		int sock = OpenNetwork();
		
		// この部分を作成
		char *buf = "こんにちは";
		write(sock, buf, sizeof(char *) * sizeof(buf));
		mySpeak(buf);
		
		CloseNetwork( sock );
		
		mySpeak( "終了します" );
		
	}
};

void AppMain(Sapie::CApplicationInitializer& initializer)
{
	// アプリケーションのインスタンスを作成して、APIへ通知する
	initializer.SetApplication(new CEchoClient());
}
