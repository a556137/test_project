// MFCtestDlg.cpp : ʵ���ļ�
//
//#include <vld.h>
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>  
//#include <crtdbg.h>  
#include "stdafx.h"
#include "MFCtest.h"
#include "MFCtestDlg.h"
#include "BlinkDetection.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <ctime>
#include <time.h>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMEOUT 200.0/fps

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMFCtestDlg �Ի���




CMFCtestDlg::CMFCtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMFCtestDlg::IDD, pParent)
	, workImg(NULL)
	, pImg(NULL)
	, disImg(NULL)
	, m_Display(-1)
	, m_dibFlag(0)
	, m_CaptFlag(0)
	, saveImg(NULL)
	, m_ImageType(0)
	, m_SaveFlag(0)
	, m_lpBmi(0)
	, m_cameraOpened(false)
	, m_videoOpened(false)
	, startFrameNum(0)
	, endFrameNum(0)
	, totalFrameNum(0)
	, m_recording(true)
	, m_pause(false)
	, m_updateSlider(false)
	, c_player(NULL)
	, framecount_test(0)
	, time_test(0.0)
{
	//_CrtSetDbgFlag(0);
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);	
	m_pauseIcon = AfxGetApp()->LoadIcon(IDI_PAUSEICON);
	//_CrtDumpMemoryLeaks();  
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(499207);  
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
}

CMFCtestDlg::~CMFCtestDlg()
{
	if (c_player != NULL)  //vfw
	{
		MCIWndClose(c_player);
		MCIWndDestroy(c_player);
		c_player=NULL;
	}

	if (m_CaptFlag)
		AbortCapture(workImg);              //  �ر���Ƶ

	if (saveImg)
		cvReleaseImage(&saveImg);           //  �ͷ�λͼ
	if (workImg)
		cvReleaseImage(&workImg);
	if (disImg)
		cvReleaseImage(&disImg);
	if (m_lpBmi)
		free(m_lpBmi);                      //  �ͷ�λͼ��Ϣ
}


void CMFCtestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTONPAUSE, m_pauseButton);
	DDX_Control(pDX, IDC_TIMESLIDER, m_timeslider);
	DDX_Control(pDX, IDC_BACKWARDBUTTON, m_backwardButton);
	DDX_Control(pDX, IDC_FORWARDBUTTON, m_forwardButton);
}

BEGIN_MESSAGE_MAP(CMFCtestDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_GETMINMAXINFO()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_VIDEOBUTTON, &CMFCtestDlg::OnBnClickedVideoButton)
	ON_BN_CLICKED(IDC_STARTBUTTON, &CMFCtestDlg::OnBnClickedStartButton)
	ON_BN_CLICKED(IDC_FINISHBUTTON, &CMFCtestDlg::OnBnClickedFinishButton)
	ON_BN_CLICKED(IDC_CAMBUTTON, &CMFCtestDlg::OnBnClickedCamButton)
	ON_BN_CLICKED(IDC_OUTPUTBUTTON, &CMFCtestDlg::OnBnClickedOutputButton)
	ON_BN_CLICKED(IDC_BUTTONPAUSE, &CMFCtestDlg::OnBnClickedButtonpause)
	ON_BN_CLICKED(IDC_FORWARDBUTTON, &CMFCtestDlg::OnBnClickedForwardbutton)
	ON_BN_CLICKED(IDC_BACKWARDBUTTON, &CMFCtestDlg::OnBnClickedBackwardbutton)
	ON_WM_HSCROLL()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_TIMESLIDER, &CMFCtestDlg::OnNMReleasedcaptureTimeslider)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_TIMESLIDER, &CMFCtestDlg::OnNMCustomdrawTimeslider)
END_MESSAGE_MAP()


// CMFCtestDlg ��Ϣ�������
//initiate dialog
BOOL CMFCtestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rect;
	//GetWindowRect(&rect);

	//int rw = rect.Width();			
	//int rh = rect.Height();

	//hFactor = (double)rect.Height()/284;
	//wFactor = (double)rect.Width()/378;

	CDC *pDC = GetDlgItem(IDC_DISFRAME)->GetDC();
	HDC m_hDC = pDC->GetSafeHdc();

	//setup size of display window
	GetDlgItem(IDC_DISFRAME) ->GetClientRect( &rect );

	CWnd *pWnd;
	pWnd = GetDlgItem(IDC_DISFRAME);
	pWnd->SetWindowPos(NULL, rect.TopLeft().x, rect.TopLeft().y, 640, 480, SWP_NOMOVE);

	//setup icons
	m_pauseIcon = AfxGetApp()->LoadIcon(IDI_PAUSEICON);
	m_pauseButton.SetIcon(m_pauseIcon);
	m_pauseIcon = AfxGetApp()->LoadIcon(IDI_FORWARDICON);
	m_forwardButton.SetIcon(m_pauseIcon);
	m_pauseIcon = AfxGetApp()->LoadIcon(IDI_BACKWARDICON);
	m_backwardButton.SetIcon(m_pauseIcon);

	//setup buttons
	EnableVideoCtrlButtons(false);

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CMFCtestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CMFCtestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
		if (1)
		{
			//CDialog::UpdateWindow();
			CDC *pDC = GetDlgItem(IDC_DISFRAME)->GetDC();
			HDC m_hDC = pDC->GetSafeHdc();

			//CRect rect;
			//GetDlgItem(IDC_DISFRAME) ->GetClientRect( &rect );

			//int rw = rect.right - rect.left;			
			//int rh = rect.bottom - rect.top;

			if (pImg!=NULL)	{	            //  �д�������ͼ��    ���һ֡
				if (m_Display==0) {           //  ��δ��ʾ
					imageClone(pImg,&saveImg);         //  ���Ƶ�����λͼ
					m_dibFlag=imageClone(saveImg,&workImg);  //  ���Ƶ�����λͼ
					if (m_CaptFlag)
					{
						cvResize(workImg, disImg);
					}

					//cvShowImage("test", disImg);
					m_ImageType=imageType(workImg);
					m_SaveFlag=m_ImageType;
					m_Display=1;
				}
			} 
			else
			{
				if (m_CaptFlag)
				{
					cvResize(workImg, disImg);
				}
			}
			IplImage *interpolationImg = disImg;
			if (m_dibFlag) {                        //  DIB �ṹ�ı�
				if (m_lpBmi)
					free(m_lpBmi);
				m_lpBmi=CtreateMapInfo(interpolationImg,m_dibFlag);
				m_dibFlag=0;
			}

			char *pBits;
			if (interpolationImg)  
				pBits=interpolationImg->imageData;

			if (interpolationImg) {                          //  ˢ�´��ڻ���
				StretchDIBits(m_hDC,
					0,0,interpolationImg->width,interpolationImg->height,
					0,0,interpolationImg->width,interpolationImg->height,
					pBits,m_lpBmi,DIB_RGB_COLORS,SRCCOPY);
			}
		}
	}
}


int  CMFCtestDlg::imageType(IplImage* p) 
{
	int	 i,j,k,bpl,n,pg[256];
	BYTE *buf;

	k=p->nChannels;
	if (k==1) {                             //  ����ֵͼ��
		for (i=0;i<256;i++) pg[i]=0;
		buf=(BYTE*)p->imageData;
		bpl=p->widthStep;
		for (i=0;i<p->height;i++) {
			for (j=0;j<p->width;j++) pg[buf[j]]++;
			buf+=bpl;
		}
		for (i=0,n=0;i<256;i++) {
			if (pg[i]) n++;
		}
		if (n==2) k=-1;                     //  ��ֵͼ��
	}
	return(k);
}


LPBITMAPINFO CMFCtestDlg::CtreateMapInfo(IplImage* workImg,int flag)
{                                           //  ����λͼ��Ϣ
	BITMAPINFOHEADER BIH={40,1,1,1,8,0,0,0,0,0,0};
	LPBITMAPINFO lpBmi;
	int      wid,hei,bits,colors,i;

	wid =workImg->width;
	hei =workImg->height;
	bits=workImg->depth*workImg->nChannels;

	if (bits>8) colors=0;
	else colors=1<<bits;

	lpBmi=(LPBITMAPINFO) malloc(40+4*colors);
	BIH.biWidth   =wid;
	BIH.biHeight  =hei;
	BIH.biBitCount=(BYTE) bits;
	memcpy(lpBmi,&BIH,40);                  //  ����λͼ��Ϣͷ

	if (bits==8) {                          //  256 ɫλͼ
		if (flag==1) {                      //  ���ûҽ׵�ɫ��
			for (i=0;i<256;i++) {
				VgaColorTab[i].rgbRed=VgaColorTab[i].rgbGreen=
					VgaColorTab[i].rgbBlue=(BYTE) i;
			}
			memcpy(lpBmi->bmiColors,VgaColorTab,1024);
		}
		else if (flag==2) {                 //  ����Ĭ�ϵ�ɫ��
			memcpy(lpBmi->bmiColors,VgaDefPal,1024);
		}
		else if (flag==3) {                 //  �����Զ����ɫ��
			memcpy(lpBmi->bmiColors,VgaColorTab,1024);
		}
	}
	return(lpBmi);
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CMFCtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCtestDlg::OnTimer(UINT nIDEvent)     //  ��ʱ����Ӧ
{
	CString testString;

	//if (time_test != 0)
	//{
	//	time_test = ((double)getTickCount() - time_test)/getTickFrequency();
	//}
	double t = (double)getTickCount();
	if (framecount_test == 0)
	{
		time_test = t;
		clock1 = clock();
	}

	int  bl=LoadImageFrame(workImg,1);      //  ������Ƶͼ��  bl����1��0   1��ʾ����λͼ

	t = ((double)getTickCount() - t)/getTickFrequency();
	testString.Format("%lf", t);
	//GetDlgItem(IDC_TEST)->SetWindowTextA(testString);

	if (bl==0) {
		OnCapAbort();                       //  �ر���Ƶ
		if (c_player != NULL)
		{
			MCIWndStop(c_player);  //vfw
			MCIWndClose(c_player); //vfw
		}

		if (getenDetect())
		{
			endFrameNum = frameNum;

			double blinkRate = CalBlinkRate(endFrameNum - startFrameNum);
			CString blinkText;
			CString windowText("Number of Blink:");
			blinkText.Format("%d", (int)(g_blinkNum));
			windowText = windowText + blinkText + CString("\r\nRate of Blink:");
			blinkText.Format("%g", (blinkRate));
			windowText = windowText + blinkText + " cpm";
			CWnd *pWnd = GetDlgItem(IDC_TEXT);
			pWnd->SetWindowText(windowText);

			if (m_recording)
			{
				RecordMorbidEvent();
			}
		}

		Reset2Norm();
	}
	else if (bl==1) {
		//Invalidate(false);                  //  ˢ�»���	
		CDC *pDC = GetDlgItem(IDC_DISFRAME)->GetDC();
		HDC m_hDC = pDC->GetSafeHdc();
		CRect rect;
		GetDlgItem(IDC_DISFRAME)->GetWindowRect( &rect );
		InvalidateRect(&rect, false);

		if (m_Video)
		{
			UpdateTimeSlider(frameNum);
		}

		unsigned long msec = frameNum*1000/fps;
		ostringstream s_timeText;
		s_timeText << msec/3600000%24 << "." << msec/60000%60 << "." << msec/1000%60 << "." << msec%1000;
		if (m_Video && totalFrameNum != 0)
		{
			unsigned long videoLength = totalFrameNum*1000/fps;
		    s_timeText << "/" << videoLength/3600000%24 << "." << videoLength/60000%60 << "." << videoLength/1000%60 << "." << videoLength%1000;
		}
		GetDlgItem(IDC_TIMESTAMP)->SetWindowTextA(CString(s_timeText.str().c_str()));

		if (getenDetect())
		{
			CString blinkText;
			CString windowText("Number of Blink:");
			blinkText.Format("%d", (int)(g_blinkNum));
			CWnd *pWnd = GetDlgItem(IDC_TEXT);
			blinkText = windowText + blinkText;
			pWnd->SetWindowText(blinkText);
		}
	}
	else if (bl==2) {
		KillTimer(IDC_TIMER);                //  �ض�ʱ��
		if (c_player != NULL)   //vfw
		{
			MCIWndPause(c_player);
		}

		AverageImage(workImg);              //  ȡ��ƽ��ֵ
		imageClone(workImg,&saveImg);       //  ����Ϊ����ͼ��

		m_ImageType=m_SaveFlag=workImg->nChannels;
		m_CaptFlag = 2;                     //  �޸���Ƶ״̬��־
		Invalidate(false);
	}

	++framecount_test;
	if (framecount_test == 30)
	{
		framecount_test = 0;
		time_test = ((double)getTickCount() - time_test)/getTickFrequency();
		double test = 0;
		test =  clock() - clock1;
		testString.Format("%lf", time_test);
		GetDlgItem(IDC_TEST)->SetWindowTextA(testString);
		double halt = 0;
	}
	//time_test = ((double)getTickCount());

	CDialog::OnTimer(nIDEvent);
}


void CMFCtestDlg::OnCapAbort()               //  �ر���Ƶ
{
	IplImage *p;

	AbortCapture(workImg);                  //  �ر���Ƶ
	if (saveImg) p=saveImg;                 //  ���ж���ͼ��
	else p=workImg;
	imageClone(p,&pImg);              //  ����Ϊԭʼͼ��

	KillTimer(IDC_TIMER);                    //  �ض�ʱ��
	m_Display=0;
	m_CaptFlag=0;                           //  ����Ƶ״̬��־
	Invalidate(false);
}

void CMFCtestDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	//CDC *pDC = GetDlgItem(IDD_MFCTEST_DIALOG)->GetDC();
	//HDC m_hDC = pDC->GetSafeHdc();

	//CRect rect;
	//GetDlgItem(IDD_MFCTEST_DIALOG) ->GetClientRect( &rect );

	//int rw = rect.right - rect.left;			
	//int rh = rect.bottom - rect.top;

	//lpMMI->ptMinTrackSize.x=rw;			//������С���Ϊ20
	//lpMMI->ptMinTrackSize.y=rh;		//������С�߶�Ϊ100
	//lpMMI->ptMaxTrackSize.x=lpMMI->ptMinTrackSize.x;
	//lpMMI->ptMaxTrackSize.y=lpMMI->ptMinTrackSize.y;
}

int  CMFCtestDlg::imageClone(IplImage* pi,IplImage** ppo)  //  ���� IplImage λͼ
{
	if (*ppo) {
		cvReleaseImage(ppo);                //  �ͷ�ԭ��λͼ
	}
	(*ppo) = cvCloneImage(pi);              //  ������λͼ
	return(1);
}

void CMFCtestDlg::OnBnClickedVideoButton()
{
	if (!m_videoOpened)
	{
		//CDC *pDC = GetDlgItem(IDC_DISFRAME)->GetDC();
		//HDC m_hDC = pDC->GetSafeHdc();

		//CRect rect;
		//GetDlgItem(IDC_DISFRAME) ->GetWindowRect( &rect );

		//int rw = rect.Width();			
		//int rh = rect.Height();

		if (disImg)
			cvReleaseImage(&disImg);
		disImg = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);

		BOOL  bl;

		CString csFilter="AVI/MOV/MPG Files(*.MOV;*.AVI;*.MPG)|*.MOV;*.AVI;*.MPG||";
		CString strFileName;

		CFileDialog FileDlg(true,NULL,NULL,OFN_HIDEREADONLY,csFilter);
		//  �ļ����̶Ի���
		if (FileDlg.DoModal()==IDOK ) {         //  ѡ�����ļ���
			strFileName = FileDlg.m_ofn.lpstrFile;
			if (FileDlg.m_ofn.nFileExtension == 0) {  //  ���ļ���׺
				strFileName = strFileName + ".avi";
				//  ���ļ���׺
			}
		} else 
			return;

		if (workImg) {
			if (pImg) {
				cvReleaseImage(&pImg);
			}
			m_Display=0;

			CClientDC dc(this);
			//dc.PatBlt(0,0,1024,768,WHITENESS);  //  �������@@@
		}

		if (strFileName == "")
		{
			return;
		}
		bl=OpenAviFile(strFileName);            //  ����Ƶ�ļ�
		if (!bl) {
			AfxMessageBox("�޷�����Ƶ�ļ�");
			return;
		}

		c_player = NULL;
		//c_player = MCIWndCreate(GetDlgItem(IDC_TEST)->GetSafeHwnd(), AfxGetInstanceHandle(),
		//	WS_CHILD | WS_VISIBLE| MCIWNDF_NOMENU| MCIWNDF_NOTIFYALL| MCIWNDF_NOPLAYBAR,
		//	strFileName);
		//MCIWndSetTimeFormat(c_player,"ms");
		//MCIWndSetActiveTimer(c_player,100);
		//MCIWndSendString(c_player, "set video off");
		//MCIWndPlay(c_player);

		if (workImg) {
			cvReleaseImage(&workImg);
		}
		workImg = cvCreateImage(cvSize(frameW,frameH), IPL_DEPTH_8U, 3);

		if (saveImg) {
			cvReleaseImage(&saveImg);
		}
		m_dibFlag=1;

		totalFrameNum = cvGetCaptureProperty(m_Video, CV_CAP_PROP_FRAME_COUNT);
		m_videoOpened = true;
		SetTimer(IDC_TIMER, floor(TIMEOUT), NULL);             //  ÿ����� 15 ��
		m_CaptFlag=1;                           //  ��Ƶ״̬��־��ʼ��

		GetDlgItem(IDC_CAMBUTTON)->EnableWindow(FALSE);
		SetDlgItemText(IDC_VIDEOBUTTON, "Close Video");
		EnableVideoCtrlButtons(true);
		m_timeslider.SetRange(0, floor((double)totalFrameNum/fps));
		m_timeslider.SetRange(0, totalFrameNum - 1);
		m_timeslider.SetPos(0);
	}
	else
	{
		OnCapAbort();
		if (c_player != NULL)   //vfw
		{
			MCIWndStop(c_player); 
			MCIWndClose(c_player);
			c_player = NULL;
		}
		Reset2Norm();
	}
}

void CMFCtestDlg::OnBnClickedStartButton()
{
	if (!getenDetect() && m_CaptFlag)
	{
		setenDetect(true);
		startFrameNum = frameNum;

		if (m_Video)
		{
			GetDlgItem(IDC_FORWARDBUTTON)->EnableWindow(false);
			GetDlgItem(IDC_BACKWARDBUTTON)->EnableWindow(false);
			GetDlgItem(IDC_TIMESLIDER)->EnableWindow(false);
		}
	}
}

void CMFCtestDlg::OnBnClickedFinishButton()
{
	if (getenDetect() && m_CaptFlag)
	{
		endFrameNum = frameNum;

		double blinkRate = CalBlinkRate(endFrameNum - startFrameNum);
		CString blinkText;
		CString windowText("Number of Blink:");
		blinkText.Format("%d", (int)(g_blinkNum));
		windowText = windowText + blinkText + CString("\r\nRate of Blink:");
		blinkText.Format("%g", (blinkRate));
		windowText = windowText + blinkText + " cpm";
		CWnd *pWnd = GetDlgItem(IDC_TEXT);
		pWnd->SetWindowText(windowText);

		if (m_recording)
		{
			RecordMorbidEvent();
		}
		setenDetect(false);

		if (m_Video)
		{
			GetDlgItem(IDC_FORWARDBUTTON)->EnableWindow(true);
			GetDlgItem(IDC_BACKWARDBUTTON)->EnableWindow(true);
			GetDlgItem(IDC_TIMESLIDER)->EnableWindow(true);
		}
	}
	else
	{
		CString windowText("Number of Blink:0\r\nRate of Blink:0 cpm");
		GetDlgItem(IDC_TEXT)->SetWindowText(windowText);
	}
	//UpdateData(1);
	//RedrawWindow();
}

void CMFCtestDlg::OnBnClickedCamButton()
{
	if (!m_cameraOpened)
	{
		if (OnStartCapture() < 0)
		{
			return;
		}		
		GetDlgItem(IDC_VIDEOBUTTON)->EnableWindow(FALSE);
		SetDlgItemText(IDC_CAMBUTTON, "Close Camera");
	}
	else
	{
		OnCapAbort();
		Reset2Norm();
	}
}

int CMFCtestDlg::OnStartCapture(void)
{
	BOOL  bl;

	if (disImg)
		cvReleaseImage(&disImg);
	disImg = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);

	if (workImg) {
		if (pImg)
		{
			cvReleaseImage(&pImg);
		}
		m_Display=0;

		//dc.PatBlt(0,0,1024,768,WHITENESS);  //  �������
	}

	bl=StartCapture();                      //  ������Ƶ
	if (!bl) {
		AfxMessageBox("����û�а�װ����ͷ��");
		return -1;
	}
	else if (bl==-1) {
		if (frameSetW>640) {
			frameSetW=640;     frameSetH=480;
			bl=StartCapture();
			if (bl==-1) {
				AfxMessageBox("�޷�������ͷ");
				return -1;
			}
		}
	}

	if (workImg) {
		cvReleaseImage(&workImg);
	}
	workImg =cvCreateImage(cvSize(frameW,frameH), IPL_DEPTH_8U, 3);
	if (saveImg) {
		cvReleaseImage(&saveImg);
	}
	m_dibFlag = 1;
	frameNum = 0;
	m_cameraOpened = true;

	//VGA 30fps
	SetTimer(IDC_TIMER, floor(TIMEOUT), NULL);             //  ÿ����� 30 ��
	m_CaptFlag=1;                           //  ��Ƶ״̬��־��ʼ��
	return 0;
}

extern MorbidEvent;

bool CMFCtestDlg::RecordMorbidEvent(void)
{
	char cAppFolderPath[MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, cAppFolderPath);

	time_t sysTime;
	struct tm *fmt;

	time(&sysTime);
	fmt = localtime(&sysTime);

	char filename[25];
	char c_num[5];

	sprintf(c_num, "%d", fmt->tm_year + 1900);
	strcpy(filename, c_num);
	strcat(filename, "-");
	sprintf(c_num, "%d", fmt->tm_mon + 1);
	strcat(filename, c_num);
	strcat(filename, "-");
	sprintf(c_num, "%d", fmt->tm_mday);
	strcat(filename, c_num);
	strcat(filename, "-");
	sprintf(c_num, "%d", fmt->tm_hour);
	strcat(filename, c_num);
	strcat(filename, "-");
	sprintf(c_num, "%d", fmt->tm_min);
	strcat(filename, c_num);
	strcat(filename, "-");
	sprintf(c_num, "%d", fmt->tm_sec);
	strcat(filename, c_num);
	strcat(filename, ".txt");

	//strcpy(filename, "test.txt");

	ofstream outfile;
	outfile.open(filename, ios::out | ios::trunc);

	ostringstream formatMassage;
	if (m_Video)
	{
		unsigned long startmsec = startFrameNum*1000/fps;
		unsigned long endmsec = endFrameNum*1000/fps;
		formatMassage << "Detection start at  " << startmsec/3600000%24 << "." << startmsec/60000%60 << "." << startmsec/1000%60 << "." << startmsec%1000 << "\n";
		formatMassage << "Detection finish at " << endmsec/3600000%24 << "." << endmsec/60000%60 << "." << endmsec/1000%60 << "." << endmsec%1000 << "\n";
	}
	formatMassage << "start of tic     end of tic     type of tic\n";
	for (vector<MorbidEvent>::const_iterator iter = eventList.begin(); iter != eventList.end(); iter++)
	{
		formatMassage << iter->StartTime.h <<"."<< iter->StartTime.m<<"."<< iter->StartTime.s <<"."<< iter->StartTime.ms;
		formatMassage << "\t ";
		formatMassage << iter->EndTime.h <<"."<< iter->EndTime.m<<"."<< iter->EndTime.s <<"."<< iter->EndTime.ms;
		formatMassage << "\t ";
		formatMassage << iter->eventType;
		formatMassage << "\n";
	}
	string eventString(formatMassage.str());

	outfile << eventString;
	outfile.close();

	return true;
}

void CMFCtestDlg::OnBnClickedOutputButton()
{
	m_recording = !m_recording;
	CString text;
	if (m_recording)
	{
		text = CString("Output:ON");
	} 
	else
	{
		text = CString("Output:OFF");
	}
	GetDlgItem(IDC_OUTPUTBUTTON)->SetWindowText(text);
}

void CMFCtestDlg::OnBnClickedButtonpause()
{
	if (m_Video)
	{
		m_pause = !m_pause;
		HICON m_icon;
		if (m_pause)
		{
			KillTimer(IDC_TIMER);
			if (c_player != NULL)  //vfw
			{
				MCIWndPause(c_player);
			}
			m_icon = AfxGetApp()->LoadIcon(IDI_PLAYICON);
			m_pauseButton.SetIcon(m_icon);
		}
		else
		{
			SetTimer(IDC_TIMER, (unsigned int)floor(TIMEOUT), NULL);
			if (c_player != NULL)
			{
				MCIWndPlay(c_player);
			}
			m_icon = AfxGetApp()->LoadIcon(IDI_PAUSEICON);
			m_pauseButton.SetIcon(m_icon);
		}
	}
}

void CMFCtestDlg::EnableVideoCtrlButtons(bool enable)
{
	GetDlgItem(IDC_BUTTONPAUSE)->EnableWindow(enable);
	GetDlgItem(IDC_BACKWARDBUTTON)->EnableWindow(enable);
	GetDlgItem(IDC_FORWARDBUTTON)->EnableWindow(enable);
	GetDlgItem(IDC_TIMESLIDER)->EnableWindow(enable);
}

void CMFCtestDlg::OnBnClickedForwardbutton()
{
	if ((frameNum + skipstep * fps) < totalFrameNum)
	{
		cvSetCaptureProperty(m_Video, CV_CAP_PROP_POS_FRAMES, (frameNum + skipstep * fps));
		frameNum += skipstep * fps;
		if (c_player != NULL)   //vfw
		{
			long pos = (long)(MCIWndGetLength(c_player)*frameNum/totalFrameNum);
			MCIWndSeek(c_player, pos);
			if (!m_pause)
			{
				MCIWndPlay(c_player);
			}
		}

		UpdateTimeSlider(frameNum);

		unsigned long time = frameNum*1000/fps;
		ostringstream s_timeText;
		s_timeText << "time:" << time/3600000 << "." << time/60000 << "." << time/1000 << "." << time%1000;
		GetDlgItem(IDC_TIMESTAMP)->SetWindowTextA(CString(s_timeText.str().c_str()));

		if (m_pause)
		{
			int  bl=LoadImageFrame(workImg,1);
			if (bl == 0)
			{
				OnCapAbort();
				if (c_player != NULL) //vfw
				{
					MCIWndStop(c_player);
					MCIWndClose(c_player);
					c_player = NULL;
				}
			}
			else if (bl == 1)
			{
				Invalidate(false);
			}
		}
	}
}

void CMFCtestDlg::OnBnClickedBackwardbutton()
{
	if (frameNum > skipstep * fps)
	{
		cvSetCaptureProperty(m_Video, CV_CAP_PROP_POS_FRAMES, (frameNum - skipstep * fps));
		frameNum -= skipstep * fps;
		if (c_player != NULL)  //vfw
		{
			long pos = (long)(MCIWndGetLength(c_player)*frameNum/totalFrameNum);
			MCIWndSeek(c_player, pos);
			if (!m_pause)
			{
				MCIWndPlay(c_player);
			}
		}

		UpdateTimeSlider(frameNum);

		unsigned long msec = frameNum*1000/fps;
		ostringstream s_timeText;
		s_timeText << msec/3600000%24 << "." << msec/60000%60 << "." << msec/1000%60 << "." << msec%1000;
		if (totalFrameNum != 0)
		{
			unsigned long videoLength = totalFrameNum*1000/fps;
			s_timeText << "/" << videoLength/3600000%24 << "." << videoLength/60000%60 << "." << videoLength/1000%60 << "." << videoLength%1000;
		}		
		GetDlgItem(IDC_TIMESTAMP)->SetWindowTextA(CString(s_timeText.str().c_str()));

		if (m_pause)
		{
			int  bl=LoadImageFrame(workImg,1);
			if (bl == 0)
			{
				OnCapAbort();
				Reset2Norm();
			}
			else if (bl == 1)
			{
				Invalidate(false);
			}
		}
	}
}

int CMFCtestDlg::UpdateTimeSlider(unsigned long currentFrameNum)
{
	unsigned int time = (unsigned int)currentFrameNum;

	m_timeslider.SetPos(time);
	return 0;
}

void CMFCtestDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

	if (m_Video)
	{
		m_pause = true;
		m_pauseIcon = AfxGetApp()->LoadIcon(IDI_PLAYICON);
		m_pauseButton.SetIcon(m_pauseIcon);
		KillTimer(IDC_TIMER);
		if (c_player != NULL)   //vfw
		{
			MCIWndPause(c_player);
		}
		m_updateSlider = true;
	}
}

void CMFCtestDlg::OnNMReleasedcaptureTimeslider(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	if (m_Video)
	{
		unsigned long frameNO = m_timeslider.GetPos() + 1;
		cvSetCaptureProperty(m_Video, CV_CAP_PROP_POS_FRAMES, frameNO);
		frameNum = frameNO;

		m_pause = false;
		m_pauseIcon = AfxGetApp()->LoadIcon(IDI_PAUSEICON);
		m_pauseButton.SetIcon(m_pauseIcon);

		SetTimer(IDC_TIMER, (unsigned int)floor(TIMEOUT), NULL);
		if (c_player != NULL)   //vfw
		{
			long pos = (long)(MCIWndGetLength(c_player)*frameNum/totalFrameNum);
			//pos = pos - pos%100;
			long err = MCIWndSeek(c_player, pos);
			if (!m_pause)
			{
				long err = MCIWndPlay(c_player);
			}
		}
		m_updateSlider = false;
	}
}


int CMFCtestDlg::Reset2Norm(void)
{
	m_cameraOpened = m_videoOpened = false;
	m_pause = false;
	m_updateSlider = false;
	startFrameNum = endFrameNum = totalFrameNum = 0; 

	setenDetect(false);
	cleanUp();

	m_pauseIcon = AfxGetApp()->LoadIcon(IDI_PAUSEICON);
	m_pauseButton.SetIcon(m_pauseIcon);
	EnableVideoCtrlButtons(false);
	m_timeslider.SetPos(0);

	GetDlgItem(IDC_VIDEOBUTTON)->EnableWindow(TRUE);
	SetDlgItemText(IDC_VIDEOBUTTON, "Open File");
	GetDlgItem(IDC_CAMBUTTON)->EnableWindow(TRUE);
	SetDlgItemText(IDC_CAMBUTTON, "Open Camera");

	return 0;
}

void CMFCtestDlg::OnNMCustomdrawTimeslider(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;

	if (m_updateSlider)
	{
		unsigned long msec = m_timeslider.GetPos()*1000/fps;
		ostringstream s_timeText;
		s_timeText << msec/3600000%24 << "." << msec/60000%60 << "." << msec/1000%60 << "." << msec%1000;
		if (totalFrameNum != 0)
		{
			unsigned long videoLength = totalFrameNum*1000/fps;
			s_timeText << "/" << videoLength/3600000%24 << "." << videoLength/60000%60 << "." << videoLength/1000%60 << "." << videoLength%1000;
		}		
		GetDlgItem(IDC_TIMESTAMP)->SetWindowTextA(CString(s_timeText.str().c_str()));
	}
}
