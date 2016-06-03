
#include "lelinkutil.h"
#include "lelinkmessage.h"

#ifdef _LINUX_
#include <sys/time.h>
#endif



#ifdef WIN32
LelinkUtil::LelinkUtil()
{
}

LelinkUtil::~LelinkUtil()
{
}

int LelinkUtil::CreateSocket()
{
    int nSocket;
    nSocket = (int) socket(PF_INET, SOCK_STREAM, 0);
    return nSocket;
}

int LelinkUtil::CreateUDPSocket()
{
	int nSocket;
    nSocket = (int) socket(PF_INET, SOCK_DGRAM, 0);
    return nSocket;
}

int LelinkUtil::CheckSocketResult(int nResult)
{
    //	check result;
    if (nResult == -1)
        return 0;
    else
        return 1;
}

int LelinkUtil::CheckSocketValid(int nSocket)
{
    //	check socket valid
    if (nSocket == -1)
        return 0;
    else
        return 1;
}

int LelinkUtil::BindPort(int nSocket, int nPort)
{
    int rc;
    int optval = 1;
    rc = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR,
                    (const char *) & optval, sizeof (int));

    if (!CheckSocketResult(rc))
	{
		return 0;
	}
        
    sockaddr_in name;
    memset(&name, 0, sizeof (sockaddr_in));
    name.sin_family = AF_INET;
    name.sin_port = htons((unsigned short) nPort);
    name.sin_addr.s_addr = INADDR_ANY;

    rc = bind(nSocket, (sockaddr *) & name, sizeof (sockaddr_in));
    if (!CheckSocketResult(rc))
	{
		return 0;
	}


	listen(nSocket,5);
   
    return 1;
}

int LelinkUtil::ConnectSocket(int nSocket, const char * szHost, int nPort)
{
	hostent *pHost=NULL;

	pHost=gethostbyname(szHost);


    if(pHost==0)
    {
        return 0;
    }

	struct in_addr in;
    memcpy(&in.s_addr, pHost->h_addr_list[0],sizeof (in.s_addr));
    sockaddr_in name;
    memset(&name,0,sizeof(sockaddr_in));
    name.sin_family=AF_INET;
    name.sin_port=htons((unsigned short)nPort);
	name.sin_addr.s_addr=in.s_addr;

	int rc=connect((SOCKET)nSocket,(sockaddr *)&name,sizeof(sockaddr_in));



	if(rc>=0)
		return 0;
	return -1;

}

int LelinkUtil::CloseSocket(int nSocket)
{
    int rc=1;
	if(!CheckSocketValid(nSocket))
	{
		return rc;
	}

	int n=shutdown((SOCKET)nSocket,SD_BOTH);
	if(n==-1)
	{
		int mmm=0;
	}

	Sleep(10);
	int nn=closesocket((SOCKET)nSocket);
	if(nn==-1)
	{
		int mmm=0;
	}


	return 0;
}

int LelinkUtil::ListenSocket(int nSocket, int nMaxQueue)
{
    int rc = 0;
    rc = listen(nSocket, nMaxQueue);
    return CheckSocketResult(rc);
}

void LelinkUtil::SetSocketNotBlock(int nSocket)
{
    //	改变文件句柄为非阻塞模式
	ULONG optval2=1;
	ioctlsocket((SOCKET)nSocket,FIONBIO,&optval2);
}

int LelinkUtil::CheckSocketError(int nResult)
{
	//	检查非阻塞套接字错误
	if(nResult>0)
		return -1;
	if(nResult==0)
		return 0;
	
	if(WSAGetLastError()==WSAEWOULDBLOCK)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int LelinkUtil::SocketWrite(int nSocket, char * pBuffer, int nLen, int nTimeout)
{
    int nOffset=0;
	int nWrite;
	int nLeft=nLen;
	int nLoop=0;
	int nTotal=0;
	
	/*
	 *	这里的nTimeout其实是重发次数，而不是实际的时间
	 */
	int nNewTimeout=nTimeout*10;
	while((nLoop<=nNewTimeout) && (nLeft>0))
	{
		nWrite=send(nSocket,pBuffer+nOffset,nLeft,0);
		if(nWrite==0)
		{
			return -1;
		}

		if(nWrite == SOCKET_ERROR)
		{
			if(WSAGetLastError()!=WSAEWOULDBLOCK)
			{	
				return -1;
			}
		}

		if(nWrite<0)
		{
			return nWrite;
		}
		
		nOffset+=nWrite;
		nLeft-=nWrite;
		nTotal+=nWrite;
		if(nLeft>0)
		{
			//	延时100ms
			SysSleep(10);
		}
		nLoop++;
	}

	return nTotal;
}

int LelinkUtil::SocketRead(int nSocket, void * pBuffer, int nLen)
{
    if(nSocket==-1)
		return -1;
	int len=0;

	len=recv((SOCKET) nSocket,(char *)pBuffer,nLen,0);

	if(len==0)
	{
		return -1;
	}

	if(len==-1)
	{

		int localError=WSAGetLastError();
		if(localError==WSAEWOULDBLOCK)
		{
			return 0;
		}

		return -1;
	}

	if(len>0)
		return len;
	else
		return -1;
}

void LelinkUtil::SysSleep(long ms)
{
	Sleep(ms);
}


int LelinkUtil::ConnectSocket(int nSocket, unsigned long serverip, int port)
{
	struct sockaddr_in ServerAddress;
    ServerAddress.sin_family=AF_INET;
    ServerAddress.sin_addr.s_addr = serverip;
    ServerAddress.sin_port=ntohs(port);

	int rc=connect((SOCKET)nSocket,(sockaddr *)&ServerAddress,sizeof(sockaddr_in));
	int nError=GetLastError();
	if(rc>=0)
		return 0;
	return -1;
}

int LelinkUtil::InitNetLib()
{

	WSADATA wsd;
	int nRet = ::WSAStartup(MAKEWORD(2, 2), &wsd);
	if(nRet != 0)
	{
		return -1;
	}

	return 0;
}

int LelinkUtil::ReleaseNetLib()
{
	//TODO
	//::WSACleanup();
	return 0;
}

int LelinkUtil::ReceiveTimer(int fd)
{
	int            iRet=0;
	fd_set         rset;
	struct timeval tv;
	
	FD_ZERO(&rset);
	FD_SET(fd,&rset);
	tv.tv_sec=0;
	tv.tv_usec=500000;
	
	iRet=select(fd+1,&rset,NULL,NULL,&tv);

	if(iRet > 0)
	{
		if (FD_ISSET(fd, &rset))
		{
			return iRet;
		}
	}

	return iRet;
}


int LelinkUtil::SockWriteUDP(int nSocket, char *szBuffer, int nLen, const char *szHost, int nPort)
{
	hostent *pHost=NULL;
	
    pHost=gethostbyname(szHost);
    if(pHost==0)
    {
        return 0;
    }
	
	struct in_addr in;
    memcpy(&in.s_addr, pHost->h_addr_list[0],sizeof (in.s_addr));
    sockaddr_in name;
    memset(&name,0,sizeof(sockaddr_in));
    name.sin_family=AF_INET;
    name.sin_port=htons((unsigned short)nPort);
	name.sin_addr.s_addr=in.s_addr;
	
	return sendto(nSocket,(char*)szBuffer,nLen,0,(LPSOCKADDR)&name,sizeof(sockaddr_in));
}


int LelinkUtil::GetLocalIP(char* szIP)
{
	char Name[255];//定义用于存放获得的主机名的变量 
	char *IP;//定义IP地址变量 
	PHOSTENT hostinfo;
	if(gethostname (Name, sizeof(Name)) == 0)
	{ 
		//如果成功地将本地主机名存放入由name参数指定的缓冲区中 
		if((hostinfo = gethostbyname(Name)) != NULL) 
		{ 
			//这是获取主机名，如果获得主机名成功的话，将返回一个指针，指向hostinfo，hostinfo 
			//为PHOSTENT型的变量，下面即将用到这个结构体 
			IP = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
			strcpy(szIP,IP);
		}
	}
	
	return 0;
}

#else //WIN32


#include <errno.h>

LelinkUtil::LelinkUtil()
{
}

LelinkUtil::~LelinkUtil()
{
}

//////////////
int LelinkUtil::CreateSocket()
{
	int nSocket;
    nSocket = (int) socket(PF_INET, SOCK_STREAM, 0);

    return nSocket;
}



int LelinkUtil::CreateUDPSocket()
{
    int nSocket;
    nSocket = (int) socket(PF_INET, SOCK_DGRAM, 0);

    return nSocket;
}

int LelinkUtil::CheckSocketResult(int nResult)
{
    //	check result;
    if (nResult == -1)
        return 0;
    else
        return 1;
}

int LelinkUtil::CheckSocketValid(int nSocket)
{
    //	check socket valid
    if (nSocket == -1)
        return 0;
    else
        return 1;
}

int LelinkUtil::BindPort(int nSocket, int nPort)
{
    int rc;
    int optval = 1;
    rc = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR,
                    (const char *) & optval, sizeof (int));

    if (!CheckSocketResult(rc))
	{
		return 0;
	}
        
    sockaddr_in name;
    memset(&name, 0, sizeof (sockaddr_in));
    name.sin_family = AF_INET;
    name.sin_port = htons((unsigned short) nPort);
    name.sin_addr.s_addr = INADDR_ANY;

    rc = bind(nSocket, (sockaddr *) & name, sizeof (sockaddr_in));
    if (!CheckSocketResult(rc))
	{
		return 0;
	}


	listen(nSocket,5);
   
    return 1;
}

int LelinkUtil::ConnectSocket(int nSocket, const char * szHost, int nPort)
{
    sockaddr_in name;
	struct hostent* host;  

	//PrintInfoLog("LelinkUtil::ConnectSocket Host Name: %s\n", szHost);  

    memset(&name,0,sizeof(sockaddr_in));
	host = gethostbyname(szHost);  
	if (host == NULL)  
	{  
		PrintErrorLog("LelinkUtil::ConnectSocket Cannot get host by hostname\n");  
		return -1; 
	}  

	const char *szIPHost = inet_ntoa(*((struct in_addr*)host->h_addr)); 
	//PrintInfoLog("LelinkUtil::ConnectSocket Host IP: %s\n", szIPHost);  

	name.sin_family =AF_INET;
	name.sin_port = htons((unsigned short)nPort);
	name.sin_addr.s_addr = inet_addr(szIPHost); 

	int rc = connect(nSocket, (struct sockaddr *)&name, sizeof(sockaddr_in));

	if(rc>=0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int LelinkUtil::CloseSocket(int nSocket)
{
    int rc=1;
	if(!CheckSocketValid(nSocket))
	{
		return rc;
	}

	int n = shutdown(nSocket, 2);
	if(n==-1)
	{
		int mmm=0;
	}

	usleep(10*1000);
	int nn = close(nSocket);
	if(nn==-1)
	{
		int mmm=0;
	}


	return 0;
}

int LelinkUtil::ListenSocket(int nSocket, int nMaxQueue)
{
	int rc = 0;
    rc = listen(nSocket, nMaxQueue);
    return CheckSocketResult(rc);
}

void LelinkUtil::SetSocketNotBlock(int nSocket)
{
	//改变文件句柄为非阻塞模式
	unsigned long optval2=1;
	ioctl(nSocket, FIONBIO, &optval2);	
}

int LelinkUtil::CheckSocketError(int nResult)
{
	//	检查非阻塞套接字错误
	if(nResult>0)
		return -1;
	if(nResult==0)
		return 0;
	
	if (errno == EINPROGRESS)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int LelinkUtil::SocketWrite(int nSocket, char * pBuffer, int nLen, int nTimeout)
{
	int nOffset=0;
	int nWrite;
	int nLeft=nLen;
	int nLoop=0;
	int nTotal=0;
	int errsv = 0;
	
	/*
	 *	这里的nTimeout其实是重发次数，而不是实际的时间
	 */
	int nNewTimeout=nTimeout*10;
	while((nLoop <= nNewTimeout) && (nLeft>0))
	{
		nWrite = send(nSocket, pBuffer + nOffset, nLeft, 0);

		errsv = errno; 

		//PrintDebugLog("LelinkUtil::SocketWrite()  send return = %d, errno = %d\n", nWrite, errsv);

		if(nWrite == 0)
		{
			return -1;
		}

		if(nWrite == -1)
		{
			if(errsv != EINPROGRESS)
			{	
				return -1;
			}
		}

		if(nWrite<0)
		{
			return nWrite;
		}
		
		nOffset+=nWrite;
		nLeft-=nWrite;
		nTotal+=nWrite;

		if(nLeft>0)
		{
			//	延时100ms
			SysSleep(10);
		}
		nLoop++;
	}

	return nTotal;
}

int LelinkUtil::SocketRead(int nSocket, void * pBuffer, int nLen)
{
    if(nSocket==-1)
		return -1;
	int len=0;
	int errsv = 0;

	len = recv(nSocket,(char *)pBuffer,nLen,0);

	errsv = errno;


	//PrintDebugLog("LelinkUtil::SocketRead() recv = %d, errno = %d\n", len, errsv);


	if(len == 0)
	{
		return -1;
	}

	if(len == -1)
	{
		if(errsv==EINPROGRESS)
		{
			return 0;
		}

		return -1;
	}

	if(len>0)
		return len;
	else
		return -1;
}

void LelinkUtil::SysSleep(long ms)
{
	usleep(ms*1000);
}


int LelinkUtil::ConnectSocket(int nSocket, unsigned long serverip, int port)
{
	struct sockaddr_in ServerAddress;
    ServerAddress.sin_family=AF_INET;
    ServerAddress.sin_addr.s_addr = serverip;
    ServerAddress.sin_port=ntohs(port);

	int rc=connect(nSocket,(sockaddr *)&ServerAddress,sizeof(sockaddr_in));
	//int nError=GetLastError();
	if(rc>=0)
		return 0;
	return -1;

	return 0;
}

int LelinkUtil::InitNetLib()
{
	//not needed
	return 0;
}

int LelinkUtil::ReleaseNetLib()
{
	//not needed
	return 0;
}


int LelinkUtil::SockWriteUDP(int nSocket, char *szBuffer, int nLen, const char *szHost, int nPort)
{
	int ret = 0;
    sockaddr_in name;
    memset(&name,0,sizeof(sockaddr_in));


	name.sin_family =AF_INET;
	name.sin_port = htons((unsigned short)nPort);
	name.sin_addr.s_addr = inet_addr(szHost); 

	ret = sendto(nSocket,(char*)szBuffer,nLen,0,(sockaddr*)&name,sizeof(sockaddr_in));

	return ret;

}


int LelinkUtil::GetLocalIP(char* szIP)
{

#ifndef CENT_OS_VMWARE

	FILE   *stream;
	char   buf[1024];

	memset( buf, '\0', sizeof(buf) );//
	stream = popen( "ifconfig | grep -A 1 'eth0' | grep 'inet addr:' | sed \'s/.*inet addr://g' | sed \'s/[[:blank:]]*Bcast.*//g\'", "r" ); 
	fread( buf, sizeof(char), sizeof(buf), stream); //
	pclose( stream );  

	if (strlen(buf) == 0) 
	{
		memset( buf, '\0', sizeof(buf) );//
		stream = popen( "ifconfig | grep -A 1 'eth1' | grep 'inet addr:' | sed \'s/.*inet addr://g' | sed \'s/[[:blank:]]*Bcast.*//g\'", "r" ); 
		fread( buf, sizeof(char), sizeof(buf), stream); //
		pclose( stream );  
	}

	if (strlen(buf) == 0) 
	{
		memset( buf, '\0', sizeof(buf) );//
		stream = popen( "ifconfig | grep -A 1 'eth2' | grep 'inet addr:' | sed \'s/.*inet addr://g' | sed \'s/[[:blank:]]*Bcast.*//g\'", "r" ); 
		fread( buf, sizeof(char), sizeof(buf), stream); //
		pclose( stream );  
	}

	strcpy(szIP, buf);

	//PrintInfoLog("LelinkUtil::GetLocalIP ip = %s", szIP);

	return 0;

#else

	int MAXINTERFACES=16; 
    char *ip = NULL; 
    int fd, intrface, retn = 0;   
    struct ifreq buf[MAXINTERFACES];   
    struct ifconf ifc;   

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)   
    {   
        ifc.ifc_len = sizeof(buf);   
        ifc.ifc_buf = (caddr_t)buf;   
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))   
        {   
            intrface = ifc.ifc_len / sizeof(struct ifreq);   

            while (intrface-- > 0)   
            {   
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))   
                {   
                    ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));   
                    break; 
                }                       
            } 
        }   
        close (fd); 
		strcpy(szIP, ip);
        return 0;   
    }  

	return -1;

#endif
}
#endif //WIN32
int LelinkUtil::GetSystemTime()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
}

int LelinkUtil::GetSystemSecond()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec;
}

int LelinkUtil::GetSystemUSecond()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_usec;
}

