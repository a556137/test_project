

int BlinkDetectionMain(IplImage *in_image, int frameNum, CascadeClassifier&,CascadeClassifier&,CascadeClassifier&);

typedef struct s_time{
	UINT32 h;
	UINT32 m;
	UINT32 s;
	UINT32 ms;
};
typedef struct MorbidEvent{
	s_time StartTime;
	s_time EndTime;
	int eventType;
};

extern bool enDetect;
extern int g_blinkNum;
extern vector<MorbidEvent> eventList;

 void setenDetect(bool);
 bool getenDetect();
 void cleanUp();
 double CalBlinkRate(unsigned long);