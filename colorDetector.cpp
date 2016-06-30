#include "opencv2/opencv.hpp"
#define BLUR_SIZE 3
#define MORPH_CLOSE_SIZE 21
#define MORPH_OPEN_SIZE 11
#define OFFSET 10

cv::Scalar g_meanColor(0,0,0);
cv::Point g_mouse(200,400);	
int g_rangeH = 10, g_rangeS = 10, g_rangeV = 10;

void on_trackbar(int);
void createTrackBars();
void colorDetection(cv::Mat src, cv::Mat &mask, cv::Scalar color, int it);
void CallBackFunc(int event, int x, int y, int flags, void* userdata);
cv::Rect drawRect(cv::Mat &frame);
void actionConfigureColors(cv::VideoCapture &cap);

int main() {
	cv::VideoCapture cap(0);
	if(!cap.isOpened())
		return -1;
	
	cv::Mat frame;
	cv::namedWindow("Frame",cv::WINDOW_AUTOSIZE);
	while(true){
		cap >> frame;
		cv::imshow("Frame",frame);
		int k = cv::waitKey(30) & 0xFF;
		if(k == 27 || k == 'q')
			break;
		else if(k == 'c')
			actionConfigureColors(cap);
	}
}

void createTrackBars(){
	cv::namedWindow("Control", cv::WINDOW_AUTOSIZE);
	cvCreateTrackbar("RANGE_H", "Control", &g_rangeH, 128, on_trackbar);
	cvCreateTrackbar("RANGE_S", "Control", &g_rangeS, 128, on_trackbar);
	cvCreateTrackbar("RANGE_V", "Control", &g_rangeV, 128, on_trackbar);
}

void on_trackbar(int){};

//Function to create a color mask and "cut" the ball in the source image
void colorDetection(cv::Mat src, cv::Mat &mask, cv::Scalar color, int it){
	cv::Mat hsv, tgt, thrs;
	//3-channel binary mask
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
	cv::GaussianBlur(hsv, hsv, cv::Size(BLUR_SIZE, BLUR_SIZE),0,0);
	cv::inRange(hsv, cv::Scalar(color[0] - g_rangeH, color[1] - g_rangeS, color[2] - g_rangeV),
              cv::Scalar(color[0]  + g_rangeH + 1 , color[1]  + g_rangeS + 1, color[2]  + g_rangeV + 1), mask);
	//image morphology transformation
	cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT,cv::Size( MORPH_CLOSE_SIZE,MORPH_CLOSE_SIZE ),cv::Point( -1, -1 ) );
  	cv::Mat element2 = cv::getStructuringElement( cv::MORPH_RECT,cv::Size( MORPH_OPEN_SIZE,MORPH_OPEN_SIZE ),cv::Point( -1, -1 ) );
  	cv::morphologyEx( mask, mask, cv::MORPH_CLOSE, element);
	cv::morphologyEx( mask, mask, cv::MORPH_OPEN, element2);

	//mask aplication
	cv::Mat mask3[] = { mask,mask,mask };
	cv::merge(mask3, 3, thrs);
	cv::bitwise_and(thrs, src, tgt);

  	std::vector<cv::Mat> channels;
  	split(hsv, channels);
	cv::imshow("Bola", tgt);
	cv::imshow("HSV", hsv);

}

void CallBackFunc(int event, int x, int y, int flags, void* userdata) {
	if (event == cv::EVENT_LBUTTONUP) {
		//Get click x y position to a global variable
		g_mouse.x = x;
		g_mouse.y = y;

	}
}

cv::Rect drawRect(cv::Mat &frame){
	cv::Point p1, p2;
	if(g_mouse.x - OFFSET >= 0)
		if (g_mouse.x + OFFSET <= frame.cols ) p1.x = g_mouse.x - OFFSET;
		else p1.x = frame.cols - OFFSET*2;
	else p1.x = 0;
	if(g_mouse.y-OFFSET>=0)
		if (g_mouse.y + OFFSET <= frame.rows ) p1.y = g_mouse.y - OFFSET;
		else p1.y = frame.rows - OFFSET*2;
	else p1.y = 0;

	p2.x = p1.x + OFFSET*2;
	p2.y = p1.y + OFFSET*2;
 	
	cv::	Rect rect(p1, p2);
	rectangle(frame, rect, cv::Scalar(255,0,0), 1, 8, 0);
	return rect;
}


void actionConfigureColors(cv::VideoCapture &cap) {
  int selectedColor = 0;
	
  cv::namedWindow("pickColors");
  cv::setMouseCallback("pickColors", CallBackFunc);

  while(true) {
	cv::Mat frame, draw;
	cap >> frame;
	draw = frame.clone();
    	//Get Region of Interest
	cv::Rect roi = drawRect(draw);;
	cv::Mat image_roi = frame(roi);
	cv::cvtColor(image_roi,image_roi, cv::COLOR_BGR2HSV, 0);

	//create a round mask
	cv::Mat mask_pickcolor(OFFSET*2,OFFSET*2,CV_8UC1,cv::Scalar(1,1,1));
	cv::circle(mask_pickcolor, cv::Point(OFFSET, OFFSET), OFFSET,
		   cv::Scalar(255,255,255), -1, 8 , 0 );
	//Find Mean of colors (Excluding outer areas)
	cv::GaussianBlur(image_roi, image_roi, cv::Size(3, 3),0,0);
	g_meanColor = cv::mean(image_roi,mask_pickcolor);

	cv::putText(draw, "Click on the screen to pick a color", cv::Point(10,30), 
		    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 0, 0), 2, cv::LINE_8, false);
	//Get HSV Color of Mouse point
	std::stringstream ss1, ss2, ss3;
	int color_hsv_H = round(g_meanColor[0]);
	ss1 << color_hsv_H;
	int color_hsv_S = round(g_meanColor[1]);
	ss2 << color_hsv_S;
	int color_hsv_V = round(g_meanColor[2]);
	ss3 << color_hsv_V;
	std::string color_hsv("["+ ss1.str() +","+ ss2.str() +","+ ss3.str() +"]");

	cv::putText(draw, color_hsv, cv::Point(10,60), cv::FONT_HERSHEY_SIMPLEX, 0.8,
		    cv::Scalar(255, 0, 0), 2, cv::LINE_8, false);

	cv::circle(draw,cv::Point(g_mouse.x,g_mouse.y),OFFSET,cv::Scalar(255,0,0),1);

	cv::imshow("pickColors", draw);
	cv::imshow("mask", mask_pickcolor);

	int k = cv::waitKey(30);
	if( k==27 || k == 'q') {
		break;
	}else if( k >= 49 && k <= 53) {
		selectedColor = k - 49;
	}
	}
	cv::destroyWindow("pickColors");
	cv::destroyWindow("mask");
}

