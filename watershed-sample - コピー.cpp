#define _USE_MATH_DEFINES

#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <math.h>

//using namespace cv;

IplImage* markers = 0, * dsp_img = 0, * wall_img = 0;
const int CV_WAITKEY_CURSORKEY_TOP = 2490368;
const int CV_WAITKEY_CURSORKEY_BOTTOM = 2621440;
const int CV_WAITKEY_CURSORKEY_RIGHT = 2555904;
const int CV_WAITKEY_CURSORKEY_LEFT = 2424832;

/* マウスイベント用コールバック関数 */
void on_mouse(int event, int x, int y, int flags, void* param)
{
    int seed_rad = 20;
    static int seed_num = 0;
    CvPoint pt;

    // (1)クリックにより中心を指定し，円形のシード領域を設定する
    if (event == CV_EVENT_LBUTTONDOWN) {
        seed_num++;
        pt = cvPoint(x, y);
        cvCircle(markers, pt, seed_rad, cvScalarAll(seed_num), CV_FILLED, 8, 0);
        cvCircle(dsp_img, pt, seed_rad, cvScalarAll(255), 3, 8, 0);
        cvShowImage("image", dsp_img);
    }
}

/* メインプログラム */
int main(int argc, char** argv){
    int* idx, i, j;
    IplImage* src_img = 0, * dst_img = 0,* wall_img = 0;

    // (2)画像の読み込み，マーカー画像の初期化，結果表示用画像領域の確保を行なう
        
    if (argc >= 3) {
        src_img = cvLoadImage(argv[1], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        wall_img = cvLoadImage(argv[2], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
    }
    if (src_img == 0 || wall_img == 0) {
        exit(-1);
    }
    dsp_img = cvCloneImage(src_img);
    dst_img = cvCloneImage(src_img);


    markers = cvCreateImage(cvGetSize(src_img), IPL_DEPTH_32S, 1);
    cvZero(markers);

    // (3)入力画像を表示しシードコンポーネント指定のためのマウスイベントを登録する
    cvNamedWindow("image", CV_WINDOW_AUTOSIZE);
    cvShowImage("image", src_img);
    cvSetMouseCallback("image", on_mouse, 0);
    cvWaitKey(0);

    // (4)watershed分割を実行する  
    cvWatershed(src_img, markers);

    // (5)実行結果の画像中のwatershed境界（ピクセル値=-1）を結果表示用画像上に表示する
    for (i = 0; i < markers->height; i++) {
        for (j = 0; j < markers->width; j++) {
            idx = (int*)cvPtr2D(markers, i, j, NULL);
            if (*idx == -1)
                cvSet2D(dst_img, i, j, cvScalarAll(255));
            if (*idx == 1) {
                //cvSet2D(dst_img, i, j, cvScalarAll(100));
            }
        }
    }

    //結果表示
    cvNamedWindow("watershed transform", CV_WINDOW_AUTOSIZE);
    cvShowImage("watershed transform", dst_img);

    //Matに直す
    cv::Mat m_dst_img = cv::cvarrToMat(dst_img);
    cv::Mat m_src_img = cv::cvarrToMat(src_img);
    cv::Mat m_wall_img = cv::cvarrToMat(wall_img);
    cv::Mat m_mask;
 

    // (6)得られた境界からmaskを作る
    cv::Size s = m_dst_img.size();
    cv::resize(m_wall_img, m_wall_img, s);
    m_mask.create(s, CV_8UC1);
    int width = m_mask.cols;
    int height = m_mask.rows;
    
    for (int i = 0; i < markers->height; i++) {
        for (int j = 0; j < markers->width; j++) {
            
            idx = (int*)cvPtr2D(markers, i, j, NULL);
            if (*idx == 1) {
                m_mask.data[i * width + j] = 255;
            }
            else {
                m_mask.data[i * width + j] = 0;
            }
        }
    }

    //最終画像の結果表示のためのレイアウトを作る
    cv::namedWindow("result",1);


    



    // (7)壁紙調整(位置、傾き、大きさ）の初期設定
    cv::Mat adj_wall = m_wall_img.clone();
    double scale = 2.5; //画面との大きさの比
    const int maxscale = 5;
    int x_move = 0;   //壁紙の横移動 右＋　左ー
    int y_move = 0;   //壁紙の縦移動
    int slope = 70;

    cv::Mat seed_wall = m_wall_img.clone();
    cv::Mat big_wall = m_wall_img.clone();


    //大きい壁紙を事前に作っておく
    cv::Mat walls[maxscale];
    for (int i = 0; i < maxscale; i++) {
        walls[i] = seed_wall;
    }
    hconcat(walls, maxscale, big_wall);
    for (int i = 0; i < maxscale; i++) {
        walls[i] = big_wall;
    }
    vconcat(walls, maxscale, big_wall);


    //cv::namedWindow("test", 1);


    //(8)壁紙作り＋画面表示
    for (;;) {
        cv::Mat changing_wall = big_wall.clone();
        



        //位置調整
        int hor_pos = changing_wall.cols;
        int ver_pos = changing_wall.rows;
        double ratio_pos = maxscale / scale;

            //横
        if (x_move*ratio_pos >= 1) {
            cv::Rect roi_pos_right(cv::Point((int)(hor_pos-1-x_move*ratio_pos),0),cv::Size((int)(x_move*ratio_pos),ver_pos));
            cv::Rect roi_pos_left(cv::Point(0,0), cv::Size((int)(hor_pos-x_move*ratio_pos),ver_pos));
            cv::Mat poss[2];
            poss[0] = changing_wall(roi_pos_right);
            poss[1] = changing_wall(roi_pos_left);
            hconcat(poss, 2, changing_wall);
        }else if (x_move*ratio_pos <= -1) {
            cv::Rect roi_pos_right(cv::Point((int)(-x_move*ratio_pos-1),0), cv::Size((int)(hor_pos + x_move*ratio_pos-1),ver_pos));
            cv::Rect roi_pos_left(cv::Point(0,0), cv::Size((int)(-x_move*ratio_pos),ver_pos));
            cv::Mat poss[2];
            poss[0] = changing_wall(roi_pos_right);
            poss[1] = changing_wall(roi_pos_left);
            hconcat(poss, 2, changing_wall);
        }
        cv::resize(changing_wall, changing_wall, big_wall.size());

            //縦
        if (y_move*ratio_pos >= 1) {
            cv::Rect roi_pos_up(cv::Point(0,0), cv::Size(hor_pos,(int)(y_move*ratio_pos)));
            cv::Rect roi_pos_down(cv::Point(0,(int)(y_move*ratio_pos)), cv::Size(hor_pos,(int)(ver_pos-y_move*ratio_pos-1)));
            cv::Mat poss[2];
            poss[0] = changing_wall(roi_pos_down);
            poss[1] = changing_wall(roi_pos_up);
            vconcat(poss, 2, changing_wall);
        }else if (y_move*ratio_pos <= -1) {
            cv::Rect roi_pos_up(cv::Point(1,1), cv::Size(hor_pos-1,(int)(ver_pos+y_move*ratio_pos-1)));
            cv::Rect roi_pos_down(cv::Point(0,(int)(ver_pos+y_move*ratio_pos-1)), cv::Size(hor_pos-1,(int)(-y_move*ratio_pos)));
            cv::Mat poss[2];
            poss[0] = changing_wall(roi_pos_down);
            poss[1] = changing_wall(roi_pos_up);
            //printf("%d %d", poss[0].rows, poss[1].rows);
            vconcat(poss, 2, changing_wall);
        }
        cv::resize(changing_wall, changing_wall, big_wall.size());




       // 傾きをつける
        int hor_slope = changing_wall.cols;
        int ver_slope = changing_wall.rows;
        double rad_slope = slope * M_PI / 180;
        int ratio_slope = 2; //短い辺が1/(1+ratio_slope*sinθ)
        double short_line = ver_slope * (1 - 1 / (1 + ratio_slope * sin(rad_slope))) / 2;
        
        if (slope > 0) {
            const cv::Point2f src_pt[] = {
                cv::Point2f(0,0),
                cv::Point2f(hor_slope - 1,0),
                cv::Point2f(0,ver_slope - 1),
                cv::Point2f(hor_slope - 1,ver_slope - 1)
            };

            const cv::Point2f dst_pt[] = {
                cv::Point2f(0,0),
                cv::Point2f((int)(hor_slope * cos(rad_slope)),(int)(short_line)),
                cv::Point2f(0,ver_slope - 1),
                cv::Point2f((int)(hor_slope * cos(rad_slope)),(int)(ver_slope-short_line-1))
            };

            // homography 行列を計算
            cv::Mat homography_matrix = getPerspectiveTransform(src_pt, dst_pt);
            warpPerspective(changing_wall, changing_wall, homography_matrix, changing_wall.size());


        }else if (slope < 0) {
            const cv::Point2f src_pt[] = {
                cv::Point2f(0,0),
                cv::Point2f(hor_slope - 1,0),
                cv::Point2f(0,ver_slope - 1),
                cv::Point2f(hor_slope - 1,ver_slope - 1)
            };

            const cv::Point2f dst_pt[] = {
                cv::Point2f((int)(hor_slope*(1-cos(rad_slope))+1),(int)(short_line)),
                cv::Point2f(hor_slope  -1,0),
                cv::Point2f((int)(hor_slope * (1 - cos(rad_slope)) + 1),(int)(ver_slope - short_line - 1)),
                cv::Point2f(hor_slope-1,ver_slope-1)
            };

            // homography 行列を計算
            cv::Mat homography_matrix = getPerspectiveTransform(src_pt, dst_pt);
            warpPerspective(changing_wall, changing_wall, homography_matrix, changing_wall.size());
        }

        cv::resize(changing_wall, changing_wall, s);




        //大きさ調整 (トリミング＋resize)

        int hor_tri = (int)(changing_wall.cols * scale / maxscale);
        int ver_tri = (int)(changing_wall.rows * scale / maxscale);

        if (slope >= 0) {
            cv::Rect roi_tri(cv::Point(0, (int)((changing_wall.rows-ver_tri)/2)), cv::Size(hor_tri, ver_tri));
            changing_wall = changing_wall(roi_tri);
            cv::resize(changing_wall, changing_wall, s);
        }
        else if (slope < 0) {
            cv::Rect roi_tri(cv::Point(0, 0), cv::Size(hor_tri, ver_tri));
            changing_wall = changing_wall(roi_tri);
            cv::resize(changing_wall, changing_wall, s);
        }
        
        //cv::Rect roi_tri(cv::Point(0,0), cv::Size(hor_tri, ver_tri));
        //changing_wall = changing_wall(roi_tri);
        //cv::resize(changing_wall, changing_wall, s);

        

        //(8)壁紙を挿入する
        changing_wall.copyTo(m_dst_img, m_mask);
        cv::namedWindow("mask", 1);
        cv::imshow("mask", m_dst_img);

        int key = cv::waitKey(10);
        if (key == 'q' || key == 'Q') {
            break;
        }else if (key == 'l') {
            x_move = x_move + 5;
        }else if (key == 'j') {
            x_move = x_move - 5;
        }
        else if (key == 'i') {
            y_move = y_move + 5;
        }
        else if (key == 'k') {
            y_move = y_move - 5;
        }

        break;
    }
    



    cvWaitKey(0);

    //cvDestroyWindow("watershed transform");
    cvReleaseImage(&markers);
    cvReleaseImage(&dsp_img);
    cvReleaseImage(&src_img);
    cvReleaseImage(&dst_img);

    return 1;
}

