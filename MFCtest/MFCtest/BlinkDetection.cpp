
#include "stdafx.h"
#include "BlinkDetection.h"
#include "CVDSCap.h"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/features2d/features2d.hpp"

#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <io.h>

using namespace std;
using namespace cv;

typedef int frameNumType;
//static const int fps = 30;

vector<MorbidEvent> eventList;

int eyeVisibleNum = 0;
int eyeInvisibleNum = 0;
int closeNum = 0;
int openNum = 0;
float blinkRate = 0;
char eyeStatus[10000];


int currentSize = 0;
int currentLeftSize = 0;

//recode vars
static const int THRESHOLD = 20;

enum e_eyeStatus {CLOSE, OPEN, UNCHANGE};
e_eyeStatus currentStatus = UNCHANGE;

typedef struct EyeData{
	int openNum;
	int closeNum;
	e_eyeStatus status;

	EyeData():openNum(0), closeNum(0), status(UNCHANGE){}
	EyeData(const EyeData &data):openNum(data.openNum), closeNum(data.closeNum), status(data.status){}
	EyeData(int open, int close, e_eyeStatus status):openNum(open), closeNum(close), status(status){}
	EyeData& operator =(const EyeData& data)
	{
		 openNum = data.openNum;
		 closeNum = data.closeNum;
		 status = data.status;
		 return *this;
	}
};

struct StatisticData{
	EyeData lefteyeData;
	EyeData righteyeData;

	StatisticData():lefteyeData(), righteyeData(){}
}previousData, currentData;

enum e_eyePos {NONE, LEFT, RIGHT, BOTH};
e_eyePos firstBlinkPos = NONE;

struct BlinkEvent{
	frameNumType start;
	frameNumType end;

	BlinkEvent():start(0), end(0){}
}leftEvent, rightEvent;

unsigned int openTimeStamp = 0, closeTimeStamp = 0; 

int previousTimeStamp;
int previousSize;
int previousLeftSize;
e_eyeStatus previousStatus;

int g_blinkNum = 0;
double g_blinkRate = 0;

bool enDetect = false;

 void setenDetect(bool tag)
{
	enDetect = tag;
}

bool getenDetect()
{
	return enDetect;
}

unsigned long frame2time(unsigned long frameNum)
{
	return frameNum*1000/fps;
}

double CalBlinkRate(unsigned long frameNum)
{
	return g_blinkNum*60*fps/frameNum;
}

int DetectAndDraw( IplImage *img, int, CascadeClassifier&, CascadeClassifier&,
                   CascadeClassifier&);

int findCorners(Mat &src, int cornerNum, vector<Point2f> &corners);

int cvText(Mat &img, const char* text, int x, int y, CvScalar color);

//static const String faceCascadeName = "d:/My Documents/Visual Studio 2008/Projects/MFCtest/haarcascades/haarcascade_frontalface_default.xml";
////"../haarcascades/haarcascade_frontalface_alt2.xml";
//static const String closeEyeCascadeName =
//"d:/My Documents/Visual Studio 2008/Projects/MFCtest/haarcascades/closed-all-eyes-20x20-8030samples-opencv100-titled-990.xml";
//static const String openEyeCascadeName =
//"d:/My Documents/Visual Studio 2008/Projects/MFCtest/haarcascades/opened-eyes-20x20-6000samples-opencv100-titled-990.xml";

CvVideoWriter *writer = NULL;

int BlinkDetectionMain(IplImage *in_image, int frameNum, CascadeClassifier &faceCascade, 
					   CascadeClassifier &closeEyeCascade, CascadeClassifier &openEyeCascade)
{
    //CvCapture* capture = 0;
    Mat frame, frameCopy, image, resizedFrame;
    //CascadeClassifier faceCascade, closeEyeCascade, openEyeCascade;

	//char cAppFolderPath[MAX_PATH];
	//GetCurrentDirectory(_MAX_PATH, cAppFolderPath);
	//GetModuleFileName(NULL,cAppFolderPath,_MAX_PATH);
	//Mat test = imread("d:/My Documents/Visual Studio 2008/Projects/MFCtest/test.jpg");
	//if(!faceCascade.load(faceCascadeName) || !closeEyeCascade.load(closeEyeCascadeName) 
	//	|| !openEyeCascade.load(openEyeCascadeName)) 
	//{
 //       //cerr << "ERROR: Could not load classifier cascade" << endl;
 //       //cerr << "Usage: facedetect [--cascade=<cascade_path>]\n"
 //       //    "   [--nested-cascade[=nested_cascade_path]]\n"
 //       //    "   [--scale[=<image scale>\n"
 //       //    "   [filename|camera_index]\n" << endl ;
 //       return -1;
	//}
	//char *videoName = "../eye.mov";
	//if (videoName)
	//	capture = cvCaptureFromFile(videoName);
	//else
	//	capture = cvCaptureFromCAM(0);
	IplImage *iplImg = in_image;

	if (!iplImg) return -1;
    //frame = iplImg;
    //if(iplImg->origin == IPL_ORIGIN_TL)
    //    frame.copyTo(frameCopy);
    //else
    //    flip(frame, frameCopy, 0);
	
	DetectAndDraw(iplImg, frameNum, faceCascade, closeEyeCascade, openEyeCascade);
	iplImg = NULL;
    return 0;
}

// 0 for close. 1 for open. 
e_eyeStatus GetBlinkStatus(const EyeData eye, const e_eyeStatus previousStatus)
{
	int validThreshold = 3;
	double evenfactor = 0.8;
	double dominantfactor = 0.4;
	double epsilon = 0.001;

	if (eye.openNum < validThreshold && eye.closeNum < validThreshold)
	{
		return previousStatus;
	}
	else if (eye.openNum > validThreshold && (eye.openNum - (evenfactor*eye.closeNum) > epsilon))
	{
		return OPEN;
	}
	else if ((dominantfactor*eye.closeNum) - eye.openNum > epsilon)
	{
		return CLOSE;
	}

	return previousStatus;
}

e_eyeStatus GetStatusChanged(const EyeData previous, const EyeData current)
{
	if (previous.status == OPEN && current.status == CLOSE)
	{
		return CLOSE;
	}
	else if (previous.status == CLOSE && current.status == OPEN)
	{
		return OPEN;
	}

	return UNCHANGE;
}

/*
left:
close  |1 0|
       |1 1|
open   |0 *|^ 
	   |1 *|
right:
close  |0 1|
       |1 1|
open   |* 0|^
       |* 1|
//*/
bool RecordFrameNum(e_eyeStatus lefteyeChange, e_eyeStatus righteyeChange, frameNumType frameNum)
{
	if (righteyeChange == UNCHANGE)
	{
		switch (lefteyeChange)
		{
		case CLOSE:
			if (rightEvent.start != 0 && leftEvent.end != 0)
			{
				break;
			}
			if (leftEvent.start == 0) //for safe
			{
				leftEvent.start = frameNum;
			}
			break;
		case OPEN:
			if (leftEvent.end == 0 && leftEvent.start != 0)
			{
				leftEvent.end = frameNum;
				if ((rightEvent.end != 0 && rightEvent.start != 0) ||
					(rightEvent.end == 0 && rightEvent.start == 0))
				{
					return true;
				}
			}
			break;
		case UNCHANGE:
			return false;
			break;
		}
		return false;
	} 

	if (lefteyeChange == UNCHANGE)
	{
		switch (righteyeChange)
		{
		case CLOSE:
			if (leftEvent.start != 0 && rightEvent.end != 0)
			{
				break;
			}
			if (rightEvent.start == 0) //for safe
			{
				rightEvent.start = frameNum;
			}
			break;
		case OPEN:
			if (rightEvent.end == 0 && rightEvent.start != 0)
			{
				rightEvent.end = frameNum;
				if ((leftEvent.end != 0 && leftEvent.start != 0) ||
					(leftEvent.end == 0 && leftEvent.start == 0))
				{
					return true;
				}
			}
			break;
		case UNCHANGE:
			break;
		}
		return false;
	}

	/*          
	except for close|1 0|
	                |1 0|
	*/
	if (lefteyeChange == righteyeChange)
	{
		if (lefteyeChange == CLOSE)
		{
			leftEvent.start = rightEvent.start = frameNum;
			return false;
		} 
		else if (lefteyeChange == OPEN && leftEvent.start != 0 && rightEvent.start != 0)
		{
			leftEvent.end = rightEvent.end = frameNum;
			return true;
		}
	}

	if (lefteyeChange != righteyeChange)
	{
		if (lefteyeChange == CLOSE)
		{
			if (rightEvent.start != 0)
			{
				leftEvent.start = rightEvent.end = frameNum;//bug
				return true;
			}
		} 
		else if (lefteyeChange == OPEN)
		{
			if (leftEvent.start != 0)
			{
				leftEvent.end = rightEvent.start = frameNum;//bug
				return true;
			}
		}
	}
	return false;
}

int DetectAndDraw(IplImage *img, int frameNum, 
                   CascadeClassifier& faceCascade, CascadeClassifier& closeEyeCascade, 
                   CascadeClassifier& openEyeCascade)
{
	if (closeEyeCascade.empty() && openEyeCascade.empty() && faceCascade.empty()) {
		if (writer != NULL)
		{
			cvWriteFrame(writer, (img));
		}
		return -1;
	}

    int i = 0;
    vector<Rect> faces;

	Mat imgMat = img;
    Mat grayImg;

    cvtColor(imgMat, grayImg, CV_BGR2GRAY);

	faceCascade.detectMultiScale(grayImg, faces, 1.3, 3, 0, Size(140, 140));

	vector<Rect> closeEye, openEye;
    for (vector<Rect>::const_iterator r = faces.begin(); r != faces.end(); r++) {
        Mat smallImgROI;
		Rect upperFace = *r;

		CvScalar color = cvScalar(0, 255, 0);

		//eyes detection
		upperFace.height = cvRound(upperFace.height >> 1);
        smallImgROI = grayImg(upperFace);
		int minEye = upperFace.width / 8;
		int maxEye = upperFace.width / 2;
        closeEyeCascade.detectMultiScale(smallImgROI, closeEye, 1.2, 0, 0, 
						Size(minEye, minEye), Size(maxEye, maxEye));
		openEyeCascade.detectMultiScale(smallImgROI, openEye, 1.2, 0, 0,
						Size(minEye, minEye), Size(maxEye, maxEye));
		//end

#define VER3
		//algorithm
#ifdef VER2
		if (!(closeEye.empty() && openEye.empty()))
		{
			EyeData lefteye, righteye;
			for (vector<Rect>::iterator iter = closeEye.begin(); iter != closeEye.end(); iter++)
			{
				if (iter->x > r->width/2)
				{
					++lefteye.closeNum;
				}
			}
			for (vector<Rect>::iterator iter = openEye.begin(); iter != openEye.end(); iter++)
			{
				if (iter->x > r->width/2)
				{
					++lefteye.openNum;
				}
			}
			righteye.openNum = openEye.size() - lefteye.openNum;
			righteye.closeNum = closeEye.size() - lefteye.closeNum;

			lefteye.status = GetBlinkStatus(lefteye, currentData.lefteyeData.status);
			righteye.status = GetBlinkStatus(righteye, currentData.righteyeData.status);

			if (eyeVisibleNum == 0)
			{
				previousData.lefteyeData = currentData.lefteyeData = lefteye;
				previousData.righteyeData = currentData.righteyeData = righteye;
			} 
			else 
			{
				previousData.lefteyeData = currentData.lefteyeData;
				previousData.righteyeData = currentData.righteyeData;
				currentData.lefteyeData = lefteye;
				currentData.righteyeData = righteye;
			}

			e_eyeStatus lefteyeChange, righteyeChange;
			lefteyeChange = GetStatusChanged(previousData.lefteyeData, currentData.lefteyeData);
			righteyeChange = GetStatusChanged(previousData.righteyeData, currentData.righteyeData);
			//bool blinkComplish = RecordFrameNum(lefteyeChange, righteyeChange, frameNum);
			if (0)
			{
				int blinkType = 0;
				e_eyePos blinkeye = NONE;
				unsigned long leftDuration = (leftEvent.end != 0)?(leftEvent.end - leftEvent.start):0;
				unsigned long rightDuration = (rightEvent.end != 0)?(rightEvent.end - rightEvent.start):0;
				unsigned long blinkDuration = 0;
				if (leftEvent.end == 0)
				{
					blinkDuration = rightDuration;
					blinkeye = RIGHT;
				}
				else if (rightEvent.end == 0)
				{
					blinkDuration = leftDuration;
					blinkeye = LEFT;
				}
				else
				{
					blinkDuration = (leftDuration > rightDuration)?leftDuration:rightDuration;
					blinkeye = (leftDuration > rightDuration)?LEFT:RIGHT;
					blinkType = (blinkDuration > fps)?2:1;
				}
				if (blinkType == 0 && blinkDuration < fps)
				{
					blinkType = 3;
				}

				if (blinkType == 1)
				{
					unsigned long startms = 0;
					unsigned long endms = 0;
					switch (blinkeye)
					{
					case LEFT:
						startms = frame2time(leftEvent.start);
						endms = frame2time(leftEvent.end);
						break;
					case RIGHT:
						startms = frame2time(rightEvent.start);
						endms = frame2time(rightEvent.end);
						break;
					default:
						break;
					}
					MorbidEvent morbidEvent = {{startms/3600000%24, startms/60000%60, startms/1000%60, startms%1000}, 
					{endms/3600000%24, endms/60000%60, endms/1000%60, endms%1000},
					blinkType};
					eventList.push_back(morbidEvent);
					++g_blinkNum;
				}
				leftEvent = BlinkEvent();
				rightEvent = BlinkEvent();
			}
			eyeVisibleNum++;
			eyeInvisibleNum = 0;
		}

#endif
#ifdef VER1
		int lefteyeSize = 0;
		int lefteyetotalSize = 0;
		int lefteyeOpen = 0, righteyeOpen = 0;
		for (vector<Rect>::iterator iter = closeEye.begin(); iter != closeEye.end(); iter++)
		{
			if (iter->x > r->width/2)
			{
				++lefteyeSize;
			}
		}
		lefteyetotalSize = lefteyeSize;
		for (vector<Rect>::iterator iter = openEye.begin(); iter != openEye.end(); iter++)
		{
			if (iter->x > r->width/2)
			{
				--lefteyeSize;
				++lefteyetotalSize;
				++lefteyeOpen;
			}
		}
		righteyeOpen = openEye.size() - lefteyeOpen;
		int righteyeSize = closeEye.size() - openEye.size() - lefteyeSize;

		if (!openEye.empty() || !closeEye.empty()) {
			if (eyeVisibleNum == 0)
			{
				previousLeftSize = currentLeftSize = lefteyeSize;
				previousSize = currentSize = closeEye.size() - openEye.size();
				previousStatus = currentStatus = (closeEye.size() > 5 && (closeEye.size() / (double)(openEye.size() + 1)) > 1.5)?CLOSE:OPEN;
			}
			//else if (lefteyetotalSize > 3 && (openEye.size() + closeEye.size() - lefteyetotalSize) > 3)
			else if (1)
			{
				previousSize = currentSize;
				currentSize = closeEye.size() - openEye.size();
				previousLeftSize = currentLeftSize;
				currentLeftSize = lefteyeSize;
				previousStatus = currentStatus;

				double factor = 0.8, largeFactor = 1, smallFactor = 1;
				int righteyeSize = openEye.size() + closeEye.size() - lefteyetotalSize;
				double templeftthreshold = (lefteyeOpen*2 > lefteyetotalSize)?((largeFactor-smallFactor)*lefteyeOpen + smallFactor*lefteyetotalSize):
					((smallFactor-largeFactor)*lefteyeOpen + largeFactor*lefteyetotalSize);
				double temprightthreshold = (righteyeOpen*2 > righteyeSize)?((largeFactor-smallFactor)*righteyeOpen + smallFactor*righteyeSize):
					((smallFactor-largeFactor)*righteyeOpen + largeFactor*righteyeSize);
				//if ((abs(currentSize - currentLeftSize - previousSize + previousLeftSize) < (openEye.size() + closeEye.size() - lefteyetotalSize)*factor &&
				//	abs(currentLeftSize - previousLeftSize) > lefteyetotalSize*factor) || 
				//	(abs(currentSize - currentLeftSize - previousSize + previousLeftSize) > (openEye.size() + closeEye.size() - lefteyetotalSize)*factor &&
				//	(abs(currentLeftSize - previousLeftSize) < lefteyetotalSize*factor)))
				int test1 = currentSize - currentLeftSize - previousSize + previousLeftSize;
				int test2 = openEye.size() + closeEye.size() - lefteyetotalSize;
				int test3 = currentLeftSize - previousLeftSize;
				int test4 = lefteyetotalSize;
				if ((abs(currentSize - currentLeftSize - previousSize + previousLeftSize) < temprightthreshold*factor &&
					abs(currentLeftSize - previousLeftSize) > templeftthreshold*factor) || 
					(abs(currentSize - currentLeftSize - previousSize + previousLeftSize) > temprightthreshold*factor &&
					(abs(currentLeftSize - previousLeftSize) < templeftthreshold*factor)))
				{
					int rest = 1;
				}

				if (
					abs(currentSize - previousSize) > THRESHOLD
					//abs(currentSize - currentLeftSize - previousSize + previousLeftSize) > THRESHOLD*2/4 &&
					//abs(currentLeftSize - previousLeftSize) > THRESHOLD*2/4
					)
				{
					currentStatus = (currentSize > previousSize)?CLOSE:OPEN;
				} 
				else
				{
					previousStatus = currentStatus;
				}
			}

			//open -> close
			if (currentStatus == CLOSE && previousStatus == OPEN)
			{
				previousTimeStamp = frameNum;
			}
			//close -> open
			if ( (frameNum - previousTimeStamp) < fps && 
				(currentStatus == OPEN && previousStatus == CLOSE) )
			{
				unsigned long startms = frame2time(previousTimeStamp);
				unsigned long endms = frame2time(frameNum);
				MorbidEvent morbidEvent = {{startms/3600000%24, startms/60000%60, startms/1000%60, startms%1000}, 
				{endms/3600000%24, endms/60000%60, endms/1000%60, endms%1000},
				1};
				eventList.push_back(morbidEvent);
				g_blinkNum++;
			}
			//printf("previousSize = %d, currentSize = %d \n",previousSize, currentSize);
			//color = (currentStatus == OPEN)?cvScalar(255, 0, 0):cvScalar(0, 255, 0);
			eyeVisibleNum++;
			eyeInvisibleNum = 0;
		}
#endif
		

		//drawing
		Rect faceRect = *r;
		if (m_Video)
		{
			ostringstream formatMassage;
			int fontHeight = 20;
			unsigned long msec = frameNum*1000/fps;
			formatMassage << msec/3600000%24 << "." << msec/60000%60 << "." << msec/1000%60 << "." << msec%1000;
			cvText(imgMat, formatMassage.str().c_str(), faceRect.x, faceRect.y - fontHeight, color);		
		}
		rectangle((imgMat), faceRect, CV_RGB(0,0,255), 2, 8, 0);
		Rect eyeRect;
		vector<Rect>::const_iterator nr;
        for (nr = closeEye.begin(); nr != closeEye.end(); nr++) {
			eyeRect = *nr;
			eyeRect.x += r->x;
			eyeRect.y += r->y;
			rectangle((imgMat), eyeRect, CV_RGB(0,255,0), 1, 8, 0);
        }
		for (nr = openEye.begin(); nr != openEye.end(); nr++) {
			eyeRect = *nr;
			eyeRect.x += r->x;
			eyeRect.y += r->y;
			rectangle((imgMat), eyeRect, CV_RGB(255,0,0), 1, 8, 0);
        }
		//end

		if (0)
		{
			vector<Point2f> corners;
			Mat eyeZone = grayImg(eyeRect);
			findCorners(eyeZone, 5, corners);

			vector<Point2f>::iterator iter;
			for (iter = corners.begin(); iter != corners.end(); iter++)
			{
				iter->x += eyeRect.x;
				iter->y += eyeRect.y;
			}
			/// Draw detected corners
			int r = 2;
			RNG rng(12345);
			for( int i = 0; i < corners.size(); i++ )
			{ circle( imgMat, corners[i], r, Scalar(rng.uniform(0,255), rng.uniform(0,255),
			rng.uniform(0,255)), -1, 8, 0 ); }
		}

		*img = imgMat;
		/*imshow("imgMat", imgMat);
		while(waitKey(0) == -1){}*/
		if (writer != NULL)
		{
			cvWriteFrame(writer, (img));
		}
		//cv::imshow("result", img);   
    }  

	//detect no face or no eye
	if (faces.empty() || (openEye.empty() && closeEye.empty()) ) {	
		eyeVisibleNum = 0;
		eyeInvisibleNum++;
		if (writer != NULL)
		{
			cvWriteFrame(writer, (img));
		}
	}
    //cv::imshow("result", img);   
	return -1;
}

int findCorners(Mat &src, int cornerNum, vector<Point2f> &corners)
{
	if( cornerNum < 1 ) { cornerNum = 1; }

	/// Parameters for Shi-Tomasi algorithm
	double qualityLevel = 0.01;
	double minDistance = 10;
	int blockSize = 3;
	bool useHarrisDetector = false;
	double k = 0.04;

	/// Copy the source image
	//Mat copy;
	//copy = src.clone();

	/// Apply corner detection
	goodFeaturesToTrack( src,
		corners,
		cornerNum,
		qualityLevel,
		minDistance,
		Mat(),
		blockSize,
		useHarrisDetector,
		k );

	/// Set the neeed parameters to find the refined corners
	Size winSize = Size( 5, 5 );
	Size zeroZone = Size( -1, -1 );
	TermCriteria criteria = TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 );

	/// Calculate the refined corner locations
	//cornerSubPix( src, corners, winSize, zeroZone, criteria );
	//imshow("sdfaad", src);

	return 1;
}

void cleanUp()
{
	g_blinkNum = 0;
	memset(eyeStatus, 0, sizeof(eyeStatus));
	//eventList.clear();
	vector<MorbidEvent>().swap(eventList);
	g_blinkRate = 0;
	previousSize = 0;
	currentSize = 0;
	previousStatus = UNCHANGE;
	currentStatus = UNCHANGE;
	previousTimeStamp = 0;
	//if(writer == NULL)
	//{
	//	writer = cvCreateAVIWriter("../blink_2.avi", CV_FOURCC('D', 'I', 'V', 'X'), 30, cvSize(1280, 720), 1);
	//}
}


int ToString_(char *str, int strLen, int blinkNum)
{
	char strNum[4];
	memset(strNum, 0, 3);
	if (blinkNum < 10) {
		strNum[0] = blinkNum + '0';
		strNum[1] = '\0';
	} else if(blinkNum >= 10 && blinkNum < 100){
		strNum[0] = (blinkNum / 10) + '0';
		strNum[1] = (blinkNum % 10) + '0';
		strNum[2] = '\0';
	}
	memset(str, 0, strLen);
	strcpy(str, "blink num per second is ");
	return 0;
}

int cvText(Mat &img, const char* text, int x, int y, CvScalar color)  
{  
	CvFont font;  
	double hscale = 1.0;  
	double vscale = 1.0;  
	int linewidth = 3;  
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC,hscale,vscale,0,linewidth);  
	//CvScalar textColor = cvScalar(255,0,0);  
	CvPoint textPos = cvPoint(x, y);  
	IplImage pImg = img;
	cvPutText(&pImg, text, textPos, &font, color);  
	return 0;
}

int ToString(char *str, int strLen, float blinkRate)
{
	char strNum[3];
	int num = blinkRate * 10;
	memset(strNum, 0, 3);
	if (num < 10) {
		strNum[0] = num + '0';
		strNum[1] = '\0';
	} else {
		strNum[0] = (num / 10) + '0';
		strNum[1] = (num % 10) + '0';
		strNum[2] = '\0';
	}
	memset(str, 0, strLen);
	strcpy(str, "Blink number is ");
	strcat(str, strNum);
	strcat(str, " per 10 secs");
	return 0;
}
