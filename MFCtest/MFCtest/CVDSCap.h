//---------------------------------------------------------
//  CVDSCap.h     2010.8.12
//---------------------------------------------------------
 
#ifndef		OPENVCCAP
#define		OPENVCCAP
 
#include <windows.h>
#include "CameraDS.h"
#include "highgui.h"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
    
//---------------------------------------------------------
//  ��Ƶ(����ͷ)�ɼ�����

extern CvCapture* m_Video;
extern IplImage*  m_Frame;                  //  ���ڴ洢һ֡

extern int        frameSetW;
extern int        frameSetH;

extern int        frameW;
extern int        frameH;
extern int        fps;
extern int        framepS;
extern int        cap_time;                 //  �������
extern int        frame_time;               //  �����ɼ�֡��
extern unsigned long		  frameNum;
  

//extern CascadeClassifier faceCascade;
//extern CascadeClassifier closeEyeCascade;
//extern CascadeClassifier openEyeCascade;

int  StartCapture(void);                    //  ������Ƶ
BOOL OpenAviFile(CString strFileName);      //  ����Ƶ�ļ�
void StartAverage(void);                    //  ��ʼƽ��
int  LoadImageFrame(IplImage* pImg,int f);  //  ������Ƶͼ��
void AverageImage(IplImage* pImg);          //  ��ͼ��ƽ��
void AbortCapture(IplImage* pImg);          //  �ر���Ƶ
BOOL SetParameter(void);                    //  ����ѡ��
BOOL LoadCascades();
 
#endif  //OPENVCCAP


