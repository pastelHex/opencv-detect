// opencv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"


using namespace std;
using namespace cv;

bool EnumerateCameras(vector<int> &camIdx);

void set_control_trackbars();

Mat* filter_red(Mat redFilter);

void delete_holes_and_dots(cv::Mat * red_hue_image);

RNG rng(12345);
int main(int argc, char** argv) {

	vector<int> v;
	//EnumerateCameras(v);

	VideoCapture cap0(0); // open the default camera
	if (!(cap0.isOpened())) // check if we succeeded
		return -1;

	namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"
	set_control_trackbars();

	namedWindow("orginal", 1);
	namedWindow("result", 1);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	for (;;) {
		Mat frame;
		Mat cont;
		Mat redFilter;
		cap0 >> frame; // get a new frame from camera
		//cv::medianBlur(frame, frame, 3);//eliminacja szumu
		cvtColor(frame, redFilter, COLOR_BGR2HSV);
		
		Mat* red_hue_image = filter_red(redFilter);
		delete_holes_and_dots(red_hue_image);

		findContours(*red_hue_image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);
		/// Draw contours
		Mat drawing = Mat::zeros((*red_hue_image).size(), CV_8UC3);
		for (int i = 0; i< contours.size(); i++)
		{
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			drawContours(drawing, contours, i, color, 1, 8, hierarchy, 0, Point());
		}

		/// Show in a window
		namedWindow("Contours", CV_WINDOW_AUTOSIZE);
		imshow("Contours", drawing);

		vector<vector<Point> > contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());
		vector<Point2f>center(contours.size());
		vector<float>radius(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
			minEnclosingCircle((Mat)contours_poly[i], center[i], radius[i]);
		}


		/// Draw polygonal contour + bonding rects + circles
		Mat drawingBox = Mat::zeros(drawing.size(), CV_8UC3);
		for (int i = 0; i< contours.size(); i++)
		{
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			drawContours(drawingBox, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
			rectangle(drawingBox, boundRect[i].tl(), boundRect[i].br(), color, 1, 8, 0);
			//circle(drawingBox, center[i], (int)radius[i], color, 2, 8, 0);
		}

		/// Show in a window
		namedWindow("Box", CV_WINDOW_AUTOSIZE);
		imshow("Box", drawingBox);


		imshow("result", *red_hue_image);
		imshow("orginal", frame); 

		red_hue_image->release();
		delete red_hue_image;

		if (waitKey(30) >= 0)
			break;
	}

	return 0;
}

void delete_holes_and_dots(cv::Mat * red_hue_image)
{
	//morphological opening (remove small objects from the foreground)
	erode(*red_hue_image, *red_hue_image, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(*red_hue_image, *red_hue_image, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing (fill small holes in the foreground)
	dilate(*red_hue_image, *red_hue_image, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(*red_hue_image, *red_hue_image, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
}

Mat* filter_red(Mat redFilter) {
	cv::Mat lower_red_hue_range;
	cv::Mat upper_red_hue_range;
	cv::inRange(redFilter, cv::Scalar(0, cvGetTrackbarPos("LowSaturation", "Control"), cvGetTrackbarPos("LowValue", "Control")),
		cv::Scalar(2, cvGetTrackbarPos("HighSaturation", "Control"), cvGetTrackbarPos("HighValue", "Control")), lower_red_hue_range);
	cv::inRange(redFilter, cv::Scalar(160, cvGetTrackbarPos("LowSaturation", "Control"), cvGetTrackbarPos("LowValue", "Control")),
		cv::Scalar(179, cvGetTrackbarPos("HighSaturation", "Control"), cvGetTrackbarPos("HighValue", "Control")), upper_red_hue_range);
	cv::Mat *red_hue_image = new Mat();
	cv::addWeighted(lower_red_hue_range, 1.0, upper_red_hue_range, 1.0, 0.0, *red_hue_image);
	return red_hue_image;
}
void set_control_trackbars()
{
	int iLowH = 0;
	int iHighH = 179;
	int iLowS = 0;
	int iHighS = 255;
	int iLowV = 0;
	int iHighV = 255;

	//Create trackbars in "Control" window
	//cvCreateTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
	//cvCreateTrackbar("HighH", "Control", &iHighH, 179);

	cvCreateTrackbar("LowSaturation", "Control", &iLowS, 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighSaturation", "Control", &iHighS, 255);
	cvSetTrackbarPos("LowSaturation", "Control", 100);
	cvSetTrackbarPos("HighSaturation", "Control", 255);

	cvCreateTrackbar("LowValue", "Control", &iLowV, 255); //Value (0 - 255)
	cvCreateTrackbar("HighValue", "Control", &iHighV, 255);
	cvSetTrackbarPos("LowValue", "Control", 50);
	cvSetTrackbarPos("HighValue", "Control", 255);
}
bool EnumerateCameras(vector<int> &camIdx)
{
	camIdx.clear();
	struct CapDriver {
		int enumValue; string enumName; string comment;
	};
	// list of all CAP drivers (see highgui_c.h)
	vector<CapDriver> drivers;
	drivers.push_back({ CV_CAP_MIL, "CV_CAP_MIL", "MIL proprietary drivers" });
	drivers.push_back({ CV_CAP_VFW, "CV_CAP_VFW", "platform native" });
	drivers.push_back({ CV_CAP_FIREWARE, "CV_CAP_FIREWARE", "IEEE 1394 drivers" });
	drivers.push_back({ CV_CAP_STEREO, "CV_CAP_STEREO", "TYZX proprietary drivers" });
	drivers.push_back({ CV_CAP_QT, "CV_CAP_QT", "QuickTime" });
	drivers.push_back({ CV_CAP_UNICAP, "CV_CAP_UNICAP", "Unicap drivers" });
	drivers.push_back({ CV_CAP_DSHOW, "CV_CAP_DSHOW", "DirectShow (via videoInput)" });
	drivers.push_back({ CV_CAP_MSMF, "CV_CAP_MSMF", "Microsoft Media Foundation (via videoInput)" });
	drivers.push_back({ CV_CAP_PVAPI, "CV_CAP_PVAPI", "PvAPI, Prosilica GigE SDK" });
	drivers.push_back({ CV_CAP_OPENNI, "CV_CAP_OPENNI", "OpenNI (for Kinect)" });
	drivers.push_back({ CV_CAP_OPENNI_ASUS, "CV_CAP_OPENNI_ASUS", "OpenNI (for Asus Xtion)" });
	drivers.push_back({ CV_CAP_ANDROID, "CV_CAP_ANDROID", "Android" });
	drivers.push_back({ CV_CAP_ANDROID_BACK, "CV_CAP_ANDROID_BACK", "Android back camera" }),
		drivers.push_back({ CV_CAP_ANDROID_FRONT, "CV_CAP_ANDROID_FRONT","Android front camera" }),
		drivers.push_back({ CV_CAP_XIAPI, "CV_CAP_XIAPI", "XIMEA Camera API" });
	drivers.push_back({ CV_CAP_AVFOUNDATION, "CV_CAP_AVFOUNDATION", "AVFoundation framework for iOS" });
	drivers.push_back({ CV_CAP_GIGANETIX, "CV_CAP_GIGANETIX", "Smartek Giganetix GigEVisionSDK" });
	drivers.push_back({ CV_CAP_INTELPERC, "CV_CAP_INTELPERC", "Intel Perceptual Computing SDK" });

	std::string winName, driverName, driverComment;
	int driverEnum;
	Mat frame;
	bool found;
	std::cout << "Searching for cameras IDs..." << endl << endl;
	for (int drv = 0; drv < drivers.size(); drv++)
	{
		driverName = drivers[drv].enumName;
		driverEnum = drivers[drv].enumValue;
		driverComment = drivers[drv].comment;
		std::cout << "Testing driver " << driverName << "...";
		found = false;

		int maxID = 100; //100 IDs between drivers
		if (driverEnum == CV_CAP_VFW)
			maxID = 10; //VWF opens same camera after 10 ?!?
		else if (driverEnum == CV_CAP_ANDROID)
			maxID = 98; //98 and 99 are front and back cam
		else if ((driverEnum == CV_CAP_ANDROID_FRONT) || (driverEnum == CV_CAP_ANDROID_BACK))
			maxID = 1;

		for (int idx = 0; idx <maxID; idx++)
		{
			VideoCapture cap(driverEnum + idx);  // open the camera
			if (cap.isOpened())                  // check if we succeeded
			{
				found = true;
				camIdx.push_back(driverEnum + idx);  // vector of all available cameras
				cap >> frame;
				if (frame.empty())
					std::cout << endl << driverName << "+" << idx << "\t opens: OK \t grabs: FAIL";
				else
					std::cout << endl << driverName << "+" << idx << "\t opens: OK \t grabs: OK";
				// display the frame
				// imshow(driverName + "+" + to_string(idx), frame); waitKey(1);
			}
			cap.release();
		}
		if (!found) cout << "Nothing !" << endl;
		cout << endl;
	}
	cout << camIdx.size() << " camera IDs has been found ";
	cout << "Press a key..." << endl; cin.get();

	return (camIdx.size()>0); // returns success
}
