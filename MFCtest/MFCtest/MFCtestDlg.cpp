// MFCtestDlg.cpp : 实现文件
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

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CMFCtestDlg 对话框




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
		AbortCapture(workImg);              //  关闭视频

	if (saveImg)
		cvReleaseImage(&saveImg);           //  释放位图
	if (workImg)
		cvReleaseImage(&workImg);
	if (disImg)
		cvReleaseImage(&disImg);
	if (m_lpBmi)
		free(m_lpBmi);                      //  释放位图信息
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


// CMFCtestDlg 消息处理程序
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

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCtestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
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

			if (pImg!=NULL)	{	            //  有磁盘输入图像    最后一帧
				if (m_Display==0) {           //  尚未显示
					imageClone(pImg,&saveImg);         //  复制到备份位图
					m_dibFlag=imageClone(saveImg,&workImg);  //  复制到工作位图
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
			if (m_dibFlag) {                        //  DIB 结构改变
				if (m_lpBmi)
					free(m_lpBmi);
				m_lpBmi=CtreateMapInfo(interpolationImg,m_dibFlag);
				m_dibFlag=0;
			}

			char *pBits;
			if (interpolationImg)  
				pBits=interpolationImg->imageData;

			if (interpolationImg) {                          //  刷新窗口画面
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
	if (k==1) {                             //  检查二值图像
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
		if (n==2) k=-1;                     //  二值图像
	}
	return(k);
}


LPBITMAPINFO CMFCtestDlg::CtreateMapInfo(IplImage* workImg,int flag)
{                                           //  建立位图信息
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
	memcpy(lpBmi,&BIH,40);                  //  复制位图信息头

	if (bits==8) {                          //  256 色位图
		if (flag==1) {                      //  设置灰阶调色板
			for (i=0;i<256;i++) {
				VgaColorTab[i].rgbRed=VgaColorTab[i].rgbGreen=
					VgaColorTab[i].rgbBlue=(BYTE) i;
			}
			memcpy(lpBmi->bmiColors,VgaColorTab,1024);
		}
		else if (flag==2) {                 //  设置默认调色板
			memcpy(lpBmi->bmiColors,VgaDefPal,1024);
		}
		else if (flag==3) {                 //  设置自定义调色板
			memcpy(lpBmi->bmiColors,VgaColorTab,1024);
		}
	}
	return(lpBmi);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCtestDlg::OnTimer(UINT nIDEvent)     //  定时器响应
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

	int  bl=LoadImageFrame(workImg,1);      //  读出视频图像  bl返回1或0   1表示传出位图

	t = ((double)getTickCount() - t)/getTickFrequency();
	testString.Format("%lf", t);
	//GetDlgItem(IDC_TEST)->SetWindowTextA(testString);

	if (bl==0) {
		OnCapAbort();                       //  关闭视频
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
		//Invalidate(false);                  //  刷新画面	
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
		KillTimer(IDC_TIMER);                //  关定时器
		if (c_player != NULL)   //vfw
		{
			MCIWndPause(c_player);
		}

		AverageImage(workImg);              //  取出平均值
		imageClone(workImg,&saveImg);       //  保存为备份图像

		m_ImageType=m_SaveFlag=workImg->nChannels;
		m_CaptFlag = 2;                     //  修改视频状态标志
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


void CMFCtestDlg::OnCapAbort()               //  关闭视频
{
	IplImage *p;

	AbortCapture(workImg);                  //  关闭视频
	if (saveImg) p=saveImg;                 //  已有冻结图像
	else p=workImg;
	imageClone(p,&pImg);              //  保存为原始图像

	KillTimer(IDC_TIMER);                    //  关定时器
	m_Display=0;
	m_CaptFlag=0;                           //  清视频状态标志
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

	//lpMMI->ptMinTrackSize.x=rw;			//设置最小宽度为20
	//lpMMI->ptMinTrackSize.y=rh;		//设置最小高度为100
	//lpMMI->ptMaxTrackSize.x=lpMMI->ptMinTrackSize.x;
	//lpMMI->ptMaxTrackSize.y=lpMMI->ptMinTrackSize.y;
}

int  CMFCtestDlg::imageClone(IplImage* pi,IplImage** ppo)  //  复制 IplImage 位图
{
	if (*ppo) {
		cvReleaseImage(ppo);                //  释放原来位图
	}
	(*ppo) = cvCloneImage(pi);              //  复制新位图
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
		//  文件存盘对话框
		if (FileDlg.DoModal()==IDOK ) {         //  选择了文件名
			strFileName = FileDlg.m_ofn.lpstrFile;
			if (FileDlg.m_ofn.nFileExtension == 0) {  //  无文件后缀
				strFileName = strFileName + ".avi";
				//  加文件后缀
			}
		} else 
			return;

		if (workImg) {
			if (pImg) {
				cvReleaseImage(&pImg);
			}
			m_Display=0;

			CClientDC dc(this);
			//dc.PatBlt(0,0,1024,768,WHITENESS);  //  清除画面@@@
		}

		if (strFileName == "")
		{
			return;
		}
		bl=OpenAviFile(strFileName);            //  打开视频文件
		if (!bl) {
			AfxMessageBox("无法打开视频文件");
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
		SetTimer(IDC_TIMER, floor(TIMEOUT), NULL);             //  每秒采样 15 次
		m_CaptFlag=1;                           //  视频状态标志初始化

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

		//dc.PatBlt(0,0,1024,768,WHITENESS);  //  清除画面
	}

	bl=StartCapture();                      //  启动视频
	if (!bl) {
		AfxMessageBox("本机没有安装摄像头！");
		return -1;
	}
	else if (bl==-1) {
		if (frameSetW>640) {
			frameSetW=640;     frameSetH=480;
			bl=StartCapture();
			if (bl==-1) {
				AfxMessageBox("无法打开摄像头");
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
	SetTimer(IDC_TIMER, floor(TIMEOUT), NULL);             //  每秒采样 30 次
	m_CaptFlag=1;                           //  视频状态标志初始化
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
	// TODO: 在此添加控件通知处理程序代码
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
