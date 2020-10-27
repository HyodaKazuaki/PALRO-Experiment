//////////////////////////////////////////////////////////
// SampleImageProcessing 画像処理プログラム				//
// コンパイル方法： makefileを使用						//
// 実行方法： ./SampleImageProcessing					//
// 動作内容： 画像処理									//
// 終了方法： 自動終了									//
//////////////////////////////////////////////////////////
#include <stdio.h>
#include <math.h>
#ifdef __MACH__
#include <opencv/highgui.h>					// CV_LOAD_IMAGE_ANYDEPTH, CV_LOAD_IMAGE_ANY_COLOR, cvLoadImage, cvSaveImage, CV_WINDOW_AUTOSIZE, cvNamedWindow, cvShowImage, cvWaitKey, cvDestroyWindowの定義
#elif
#include <highgui.h>					// CV_LOAD_IMAGE_ANYDEPTH, CV_LOAD_IMAGE_ANY_COLOR, cvLoadImage, cvSaveImage, CV_WINDOW_AUTOSIZE, cvNamedWindow, cvShowImage, cvWaitKey, cvDestroyWindowの定義
#endif

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

///////////////////////////////////////////////////////////////////////////
// HSV表色系において色相がlow~highの間で、彩度がlows以上の画素数を数え、色を白くし、重心に丸を打つ
// 引数：	img: 	解析するBMP画像ポインタ
//			low: 	色相の下限値
// 			high:	色相の上限値
//			lows:	彩度の下限値
// 返り値：該当する画素数
///////////////////////////////////////////////////////////////////////////
long HueCount( IplImage* img, double low, double high, double lows )
{
	long res = 0;
	double rgb[3], hsv[3];
	bool invFlag = false;
	
	if( high < low ){
		invFlag = true;
	}
	
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
		}
	}
	if( res != 0 ){
		grav.x /= res;
		grav.y /= res;
		cvCircle( img, grav, 3, CV_RGB( 255, 0, 0 ) );
	}
	
	return res;
}

///////////////////////////////////////////////////////////////////////////
// カラーパレットの表示
// 引数：なし
// 返り値：なし
///////////////////////////////////////////////////////////////////////////
void ShowHSVspace()
{
	IplImage* test_img = cvCreateImage( cvSize( 360, 256 ), IPL_DEPTH_8U, 3 );	// 画像の新規作成
	double hsv[3], rgb[3];
	for( int y = 0; y < 256; y ++ ){
		for( int x = 0; x < 360; x++ ){	
			hsv[0] = x;
			hsv[1] = y / 255.0;
			hsv[2] = 1.0;
			HSVtoRGB( hsv, rgb );
			int pos = test_img->widthStep * y + x * 3;
			test_img->imageData[pos    ] = cvRound (rgb[2]*255);	// Blueのセット
			test_img->imageData[pos + 1] = cvRound (rgb[1]*255);	// Greenのセット
			test_img->imageData[pos + 2] = cvRound (rgb[0]*255);	// Redのセット
		}
	}
			
	cvNamedWindow( "Test_Image", CV_WINDOW_AUTOSIZE );
	cvShowImage( "Test_Image", test_img );
	cvWaitKey( 0 );
	
	cvDestroyWindow( "Test_Image" );	
	cvReleaseImage( &test_img );
}

void constHSV(IplImage* img, double h, double s, double v) {
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
			if(h < 0){
				hsv[1] = s;
				hsv[2] = v;
			}
			else if(s < 0){
				hsv[0] = h;
				hsv[2] = v;
			}
			else{
				hsv[0] = h;
				hsv[1] = s;
			}
			HSVtoRGB(hsv, rgb);
			img->imageData[pos + 0] = cvRound(rgb[2] * 255.0);
			img->imageData[pos + 1] = cvRound(rgb[1] * 255.0);
			img->imageData[pos + 2] = cvRound(rgb[0] * 255.0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
// メイン関数
///////////////////////////////////////////////////////////////////////////
int main( void )
{
	ShowHSVspace();
	
	IplImage* src_img = cvLoadImage( "COMM.bmp", CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR );
	if( src_img == 0 ){
		return 1;
	}
	
	IplImage* post_img = cvCloneImage( src_img );
	if( post_img->width != src_img->width || post_img->height != src_img->height ){
		return 1;
	}
	
//	cvSaveImage( BMP_FILE_NAME, src_img );
	
	cvNamedWindow( "Befor_Image", CV_WINDOW_AUTOSIZE );
	cvShowImage( "Befor_Image", src_img );
	cvWaitKey( 0 );
	
	printf( "%d\n", HueCount( post_img, 330.0, 360.0, 0.8 ) );
	constHSV(post_img, -1.0, 1.0, 1.0);
	cvNamedWindow( "After_Image", CV_WINDOW_AUTOSIZE );
	cvShowImage( "After_Image", post_img );
	cvWaitKey( 0 );
	
	cvDestroyWindow( "Before_Image" );
	cvDestroyWindow( "Afrer_Image" );
	
	cvReleaseImage( &src_img );
	cvReleaseImage( &post_img );
	return 0;
}

