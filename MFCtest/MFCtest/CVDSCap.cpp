//---------------------------------------------------------
//  CVDSCap.cpp     2010.8.12
//---------------------------------------------------------

#include "stdafx.h"
#include "cv.h"                             //  OpenCV �ļ�ͷ
#include "highgui.h"
#include "BlinkDetection.h"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <sys/stat.h>
#include <io.h>
//---------------------------------------------------------
//  ��Ƶ(����ͷ)�ɼ�����
 
CCameraDS  camera;

CvCapture* m_Video;                         //  ��Ƶָ��
IplImage*  m_Frame;                         //  OpenCV λͼָ��
IplImage** m_List;                          //  λͼָ���

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

int	       cap_shift=2;      //  ȡƽ������λ��                 1-2-3 
int        cap_num  =4;      //  ������������Ŀ(��������ֵ���) 2-4-8
int        frame_time = 4;
unsigned long		   frameNum = 0;

int        frameSetW=640;                   //  �ɼ��ֱ�������ֵ
int        frameSetH=480;

int        frameW;
int        frameH;
int        fps;
int        cap_time;                        //  �����־

 CascadeClassifier faceCascade;
 CascadeClassifier closeEyeCascade;
 CascadeClassifier openEyeCascade;

static const String faceCascadeName = "../haarcascades/haarcascade_frontalface_default.xml";
//"../haarcascades/haarcascade_frontalface_alt2.xml";
static const String closeEyeCascadeName =
//"../haarcascades/haarcascade_lefteye_2splits.xml";
"../haarcascades/closed-all-eyes-20x20-8030samples-opencv100-titled-990.xml";
static const String openEyeCascadeName =
//"../haarcascades/haarcascade_righteye_2splits.xml";
"../haarcascades/opened-eyes-20x20-6000samples-opencv100-titled-990.xml";

int  StartCapture(void)                     //  ������Ƶ
{
	int cam_count,i;

	//������ȡ����ͷ��Ŀ
	cam_count = CCameraDS::CameraCount();
	printf("There are %d cameras.\n", cam_count);


	//��ȡ��������ͷ������
	for(i=0; i < cam_count; i++)
	{
		char camera_name[1024];  
		int retval = CCameraDS::CameraName(i, camera_name, sizeof(camera_name) );

		if(retval >0)
			printf("Camera #%d's Name is '%s'.\n", i, camera_name);
		else
			printf("Can not get Camera #%d's name.\n", i);
	}


	if(cam_count==0)
		return(false);

	//�򿪵�һ������ͷ
	//if(! camera.OpenCamera(0, true)) //��������ѡ�񴰿�
	if(! camera.OpenCamera(0, false, frameSetW,frameSetH)) //����������ѡ�񴰿ڣ��ô����ƶ�ͼ���͸�
	{
		fprintf(stderr, "Can not open camera.\n");
		camera.CloseCamera();
		return(-1);
	}

	frameW=frameSetW;    frameH=frameSetH;  //  ȡ���÷ֱ���
	fps = camera.GetFps();
	cap_time=0;                             //  �����־��ʼ��
 
	//-------------------------------------------
	cap_num=(1<<cap_shift);                 //  ȷ��������λͼ��

	LoadCascades();
	m_List=(IplImage**) malloc(cap_num*sizeof(IplImage*));
	for (i=0;i<cap_num;i++) {
		m_List[i]=cvCreateImage(cvSize(frameW,frameH), IPL_DEPTH_8U, 3);
		                                    //  ����λͼ������
	}
	frame_time=0;
	return(true);
}

BOOL OpenAviFile(CString strFileName)       //  ����Ƶ�ļ�
{
	m_Video = cvCreateFileCapture(strFileName);
	if (!m_Video) return(false);

 	m_Frame=cvQueryFrame(m_Video);
	frameNum = 1;
	frameW =(int) cvGetCaptureProperty(m_Video,CV_CAP_PROP_FRAME_WIDTH);
	frameH =(int) cvGetCaptureProperty(m_Video,CV_CAP_PROP_FRAME_HEIGHT);
	fps =(int) cvRound(cvGetCaptureProperty(m_Video,CV_CAP_PROP_FPS));

	LoadCascades();

 	cap_time=0;                             //  ��־��ʼ��

	//-------------------------------------------
	m_List=(IplImage**) malloc(cap_num*sizeof(IplImage*));
	for (int i=0;i<cap_num;i++) {
		m_List[i]=cvCreateImage(cvSize(frameW,frameH), IPL_DEPTH_8U, 3);
	}
	frame_time=0;
	return(true);
}

BOOL LoadCascades()
{
	TCHAR exeFullPath[MAX_PATH];
	GetModuleFileName(NULL,exeFullPath, MAX_PATH);
	CString szfolder = exeFullPath;
	int pos = szfolder.ReverseFind('\\');
	szfolder = szfolder.Left(pos+1);
	SetCurrentDirectory(szfolder);

	char cAppFolderPath[MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, cAppFolderPath);
	//GetModuleFileName(NULL,cAppFolderPath,_MAX_PATH);
	if(!faceCascade.load(faceCascadeName) || !closeEyeCascade.load(closeEyeCascadeName) 
		|| !openEyeCascade.load(openEyeCascadeName)) 
	{
		//cerr << "ERROR: Could not load classifier cascade" << endl;
		//cerr << "Usage: facedetect [--cascade=<cascade_path>]\n"
		//    "   [--nested-cascade[=nested_cascade_path]]\n"
		//    "   [--scale[=<image scale>\n"
		//    "   [filename|camera_index]\n" << endl ;
		return false;
	}
	return true;
}

void StartAverage(void)
{
	frame_time=cap_num;
}

int  LoadImageFrame(IplImage* pImg,int f)   //  ������Ƶͼ��
{
	int  bl;

	if (m_Video) {
 		m_Frame=cvQueryFrame(m_Video);      //  ȡ��λͼ
	}
	else {
		m_Frame=camera.QueryFrame();
	}
	frameNum++;

	if (!m_Video)
	{
		cvFlip(m_Frame, m_Frame, 0);
	}
	if (getenDetect())
	{
		BlinkDetectionMain(m_Frame, frameNum, faceCascade, closeEyeCascade, openEyeCascade);
	}
	else
	{
		cleanUp();
	}
	cvFlip(m_Frame, m_Frame, 0);

	//frame_time == 0
	if (m_Frame) {
		if (frame_time==0) bl=1;
		else {                              //  ��ͼ��ƽ������
			frame_time--;
			memcpy(m_List[frame_time]->imageData,m_Frame->imageData,
					m_Frame->imageSize);    //  ����λͼ
			if (frame_time==0) bl=2;
			else bl=1;
		}
	}
	else bl=0;

	if ((bl==1)&&(f==1)) {
		memcpy(pImg->imageData,m_Frame->imageData,
				m_Frame->imageSize);        //  ����λͼ
 		cap_time++;                         //  �޸ı�־
	}
	return(bl);
}

void AverageImage(IplImage* pImg)           //  ��ͼ��ƽ��
{                                           //  OpenCV���ʵ��
	IplImage* pImg32f = NULL;
	IplImage* pImg32s = NULL;
	int   i,j,k,*pi;
	BYTE  *pb;

 	pImg32f = cvCreateImage(cvGetSize(pImg),
				IPL_DEPTH_32F,pImg->nChannels);
 	pImg32s = cvCreateImage(cvGetSize(pImg),
				IPL_DEPTH_32S,pImg->nChannels);

	cvZero(pImg32f);
	for (i=0;i<cap_num;i++) {
		cvAcc(m_List[i],pImg32f,NULL);      //  �������ݵ���
	}
	cvConvertScale(pImg32f, pImg32s, 1.0, 0);  //  ת��Ϊ����

	k=min(pImg->width,pImg32s->width);
	for (i=0;i<pImg->height;i++) {
		pb=(BYTE*) (pImg->imageData+i*pImg->widthStep);
		pi=(int*)  (pImg32s->imageData+i*pImg32s->widthStep);
		for (j=0;j<k*pImg->nChannels;j++) 
			pb[j]=(BYTE) (pi[j]>>cap_shift);  //  pi[j]/cap_num
	}

	cvReleaseImage(&pImg32f);
	cvReleaseImage(&pImg32s);
}

void AbortCapture(IplImage* pImg)           //  �ر���Ƶ
{
	if (cap_time==0) {                      //  ������δ����
		LoadImageFrame(pImg,1);             //  ������Ƶͼ��
	}
	if (m_Video) {
 		cvReleaseCapture(&m_Video);         //  �ͷ���Ƶ
		m_Video = NULL;
	}
	else {
		camera.CloseCamera();               //  �ر�����ͷ
	}

	//-------------------------------------------
	for (int i=0;i<cap_num;i++) {
 		cvReleaseImage(&m_List[i]);         //  �ͷ�λͼ������
	}
	free(m_List);
	frame_time=0;
}

BOOL SetParameter(void)                     //  ����ѡ��
{
	int cam_count;

	//������ȡ����ͷ��Ŀ
	cam_count = CCameraDS::CameraCount();
	printf("There are %d cameras.\n", cam_count);


	//��ȡ��������ͷ������
	for(int i=0; i < cam_count; i++)
	{
		char camera_name[1024];  
		int retval = CCameraDS::CameraName(i, camera_name, sizeof(camera_name) );

		if(retval >0)
			printf("Camera #%d's Name is '%s'.\n", i, camera_name);
		else
			printf("Can not get Camera #%d's name.\n", i);
	}


	if(cam_count==0) 
	{
		fprintf(stderr, "Can not open camera.\n");
 		return(false);
	}

	if( camera.OpenCamera(0, true))         //  ��������ѡ�񴰿�
	{
		frameSetW=camera.GetWidth();        //  ��ȡ���÷ֱ���
		frameSetH=camera.GetHeight();
		camera.CloseCamera();               //  �ر�����ͷ
		return(true);
	}
	else
	{
		fprintf(stderr, "Can not open camera.\n");
 		return(false);
	}
}

