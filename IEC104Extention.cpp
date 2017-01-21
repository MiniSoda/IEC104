#include "stdafx.h"
#include "IEC104Extention.h"

iec104ex_class::iec104ex_class()
{
	mEnding = false;
	mAllowConnect = true;
	bIntegralTotal = FALSE;
	mLog.activateLog();
	mLog.dontLogTime();
	m_timeout = -1;

	//连接事件，用于管理线程连接 
	//手动重置，初始FALSE
	hConnectEvt = CreateEvent(NULL,TRUE,FALSE,NULL);
	//程序退出
	hAPPExit = CreateEvent(NULL,TRUE,FALSE,NULL);

	//每秒执行的计时器
	AfxBeginThread( threadStateFunc, this );
	//监听端口
	AfxBeginThread( threadListening, this );
}

void iec104ex_class::connectTCP()
{
	

	//初始化Socket
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
	m_TCPSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( INVALID_SOCKET == m_TCPSocket )
	{
		char info[255];
		sprintf_s( info, "Error at socket(): %d\n", WSAGetLastError() );	
		mLog.pushMsg( info );
		WSACleanup();
		return;
	}


	SOCKADDR_IN addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons( getPortTCP() );
	addr.sin_addr.S_un.S_addr = inet_addr( getSecondaryIP() );
	int iError = connect(m_TCPSocket, (sockaddr *)&addr, sizeof(addr) );
	if( iError <0 )
	{
		char szInfo[255] = {0};
		sprintf_s(szInfo,"Error in connect() at %d %s, ErrorCode: %d",__FUNCTION__, __FILE__, WSAGetLastError());
		mLog.pushMsg(szInfo);
		return;
	}
	enable_connect();
	onConnectTCP();
	return;
}

void iec104ex_class::disconnectTCP()
{
	int nRet = shutdown(m_TCPSocket,SD_RECEIVE);
	if( SOCKET_ERROR == nRet )
	{
		char szText[255];
		sprintf_s( szText, "ERROR occurs in shutdown(),ERRORCODE: %d ",WSAGetLastError());
		mLog.pushMsg( szText );
		return;
	}
	disable_connect();
	closesocket(m_TCPSocket);
	WSACleanup();
	m_TCPSocket = NULL;
	onDisconnectTCP();
	return;
}

int iec104ex_class::readTCP( char * buf, int szmax )
{
	int nRet = recv( m_TCPSocket, buf, szmax, 0);
	return nRet;
}

void iec104ex_class::sendTCP( char * data, int size )
{
	send( m_TCPSocket, data, size, 0 );
}

void iec104ex_class::interrogationActConfIndication()
{

}

void iec104ex_class::interrogationActTermIndication()
{
	if( bIntegralTotal )
	{ 
		solicitIntegratedTotal();
		bITreceived = false;
	}else 
		setGICountDown( m_timeout );
	return;
}

void iec104ex_class::commandActConfIndication( iec_obj *obj )
{

}

void iec104ex_class::commandActTermIndication( iec_obj *obj )
{

}

void iec104ex_class::integraltotalActConfIndication()
{
	if (!bITreceived)
	{
		ReadIntegratedTotal();
	}
}

void iec104ex_class::integraltotalActTermIndication()
{
	setGICountDown( m_timeout );
	return;
}

//send msg to main msg_queue to process data
//be careful about the pointers which may exceed the range of array
//不能使用postmessage， 除非将obj拷贝走
void iec104ex_class::dataIndication( iec_obj *obj, int numpoints )
{
	HWND hMainWindow = AfxGetApp()->m_pMainWnd->m_hWnd;
	::SendMessage( hMainWindow, WM_INFONOTIFY, (WPARAM )obj, (LPARAM)numpoints);
	return;
}

void iec104ex_class::startListening()
{
	AfxBeginThread( threadListening, this );
}

UINT iec104ex_class::threadStateFunc( LPVOID lParam )
{
	iec104ex_class *pIECex = (iec104ex_class *)lParam;
	HANDLE hAPPExit = pIECex->hAPPExit;

	while( true)
	{
		DWORD dwReturns = WaitForSingleObject( hAPPExit, 1000 );
		if (dwReturns == WAIT_TIMEOUT)
		{
			pIECex->onTimerSecond();
		}
		else if( dwReturns == WAIT_OBJECT_0 )
		{
			pIECex->disconnectTCP();
			break;
		}
	}
	return 0;
}

iec104ex_class::~iec104ex_class()
{
	mLog.deactivateLog();
}

UINT iec104ex_class::threadListening( LPVOID lParam )
{
	iec104ex_class *pIECex = (iec104ex_class *)lParam;
	HANDLE hAPPExit = pIECex->hAPPExit;
	HANDLE hWaitObjects[2] = { hAPPExit, pIECex->hConnectEvt };

	while( true )
	{
		DWORD dwReturn = WaitForMultipleObjects( 2, hWaitObjects, false, INFINITE ) ;
		if( dwReturn == WAIT_OBJECT_0 )
		{
			pIECex->disconnectTCP();
			break;
		}else if( dwReturn == WAIT_OBJECT_0+1 )
		{
			char buf[1024];
			memset( buf, 0 ,1024);
			int BytesInQue = recv(pIECex->m_TCPSocket,(char*)buf, 1024, MSG_PEEK);
			if( BytesInQue <= 0 )
			{
				//do something to disconnect
				 pIECex->disconnectTCP();
			}
			pIECex->packetReadyTCP();
		}
	}
	return 0;
}


void iec104ex_class::enable_connect()
{
	SetEvent(hConnectEvt);
	return;
}

void iec104ex_class::disable_connect()
{
	ResetEvent(hConnectEvt);
	return;
}

void iec104ex_class::OnExit()
{
	SetEvent(hAPPExit);
}

void iec104ex_class::setIntegralTotal( BOOL flag )
{
	bIntegralTotal = flag;
	return;
}

BOOL iec104ex_class::getIntegralTotal( )
{
	return bIntegralTotal;
}

void iec104ex_class::setCountDown( const int timeout )
{
	m_timeout = timeout;
	return;
}


