#include "stdafx.h"
#include "iec104_class.h"

#ifndef __IEC104EXTENTION_H
#define __IEC104EXTENTION_H

#define WM_INFONOTIFY WM_USER + 12

class iec104ex_class:public iec104_class{

public:
	explicit iec104ex_class();
	~iec104ex_class();

	void terminate();
	void disable_connect();
	void enable_connect();

	void setSocket( SOCKET sock );
	SOCKET getSocket();

	void OnTickCount();
	void startMonitor();
	void OnExit();
	void setCountDown(const int timeout);
	void setIntegralTotal( BOOL flag );
	BOOL getIntegralTotal( );

private:
	//数据定义
	SOCKET m_TCPSocket;
	int m_timeout;
	BOOL bIntegralTotal;
	// redefine for iec104_class
	void connectTCP();
	void disconnectTCP();
	int readTCP( char * buf, int szmax );
	void sendTCP( char * data, int sz );
	void interrogationActConfIndication();
	void interrogationActTermIndication();
	void integraltotalActConfIndication();
	void integraltotalActTermIndication();
	void commandActConfIndication( iec_obj *obj );
	void commandActTermIndication( iec_obj *obj );
	void dataIndication(iec_obj *obj, int numpoints);
	bool mEnding;
	bool mAllowConnect;
	void startListening();


private:
	static UINT threadStateFunc( LPVOID lParam );
	static UINT threadListening( LPVOID lParam );
	HANDLE hConnectEvt;
	HANDLE hAPPExit;
};



#endif