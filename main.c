#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <winsock.h>

#define ID_PING         1000
#define ID_SERVER_1     1001
#define ID_SERVER_2     1002
#define ID_SERVER_3     1003
#define ID_SERVER_4     1004

char szClassName[ ] = "Windows_MCPE_Server";
WSADATA wsaData;
SOCKET  ServerSock;
struct sockaddr_in server;
struct sockaddr_in client;


#define MAGICNUMBER {0,0xff,0xff,0,0xfe,0xfe,0xfe,0xfe,0xfd,0xfd,0xfd,0xfd,0x12,0x34,0x56,0x78}

typedef struct PACKED REQUEST_UNCONNECTED_PING_
{
    UCHAR code;
    UCHAR time[8];
    UCHAR magic[16];    
}REQUEST_UNCONNECTED_PING;

REQUEST_UNCONNECTED_PING PACKED_UNCONNECTED_PING={1,{0},MAGICNUMBER};


typedef struct PACKED RESPONCE_UNCONNECTED_PONG_
{
    UCHAR code;
    UINT64 PingID;
    UINT64 ServerID;
    UCHAR MAGIC[16];    
    USHORT NameLen;
}RESPONCE_UNCONNECTED_PONG;

typedef struct fields
{
    UCHAR DATA[1024];
    UCHAR *PlayerName;
    UCHAR *Version;
    UCHAR *cPlayers;
    UCHAR *mPlayers;
    UCHAR *WorldName;
    UCHAR *GameMode;
    UCHAR *Port;   
}FIELDS;


HINSTANCE ins;
HWND hWnd;
HWND Start;
HWND Bind;
HWND PingPort;
HWND ExtPortStart,ExtPortEnd;
HWND IntPortStart,IntPortEnd;
HWND ToIP;
HWND Name,World,Version,Mode;
HWND S[4];
void CenterOnScreen();

FIELDS D[4];
UCHAR PORT[8];

int DecodePacket(UCHAR *buffer, int len, FIELDS *S,UCHAR *IP)
{
   int ret,n; UCHAR *p, *t,*D;
   p=buffer;
   t=p;
   //remove color TEXT chars
   while(t[0]!=0)
   {
       if(t[0]==0xA7){ t+=2; }
       else if(isprint(t[0])){ *p++=*t++; }
        else{ t++; }                
   }
   p[0]=0;
   //copy IP to FIELDS DATA
   D=S->DATA; //pointer to data
   memset(S->DATA,0,1024);
   n=sprintf(D,"%s\0",IP);
   D+=(n+1); 
   
   p=strstr(buffer,"MCPE"); if(!p) return 0;
   //copy the buffer to DATA
   memcpy(D,p,strlen(p));
   t=strchr(D,';');         if(!t) return 0;
   t[0]=0;   t++;           S->PlayerName=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           //drop it 
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           S->Version=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           S->cPlayers=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           S->mPlayers=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           //drop it
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           S->WorldName=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           S->GameMode=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           //drop it
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;   t++;           S->Port=t;
   t=strchr(t,';');         if(!t) return 0;
   t[0]=0;                  //drop it
   
return 1;
}

int net_isrecv(int fd, int sec )
{
    int ret;
    fd_set read_fds;
    struct timeval tv; 
    if(fd <1)
      return -1;
    FD_ZERO( &read_fds );
    FD_SET( fd, &read_fds );
    tv.tv_sec  = sec;
    tv.tv_usec = 0;
    ret = select(fd+1, &read_fds, NULL, NULL, &tv );
    if(ret == SOCKET_ERROR)
       return -1; //error
    if(FD_ISSET( fd, &read_fds ))
        return 1; //ok have data
  return 0;
}

void Ping()
{
  int port,ret,n;
  int True = 1;
  UCHAR *p;
  EnableWindow(Start,0);
  //GetPort to PING  
  memset(PORT,0,8);
  if(GetWindowText(PingPort,PORT,8)<1)
  {
        MessageBox(hWnd, "Port can't be empty\nFor Android = 19132","Server Port",MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        EnableWindow(Start,1);
        return; 
  }                              
  port=atoi(PORT);
  if(port>65535)
  {
        MessageBox(hWnd, "Invalid Port Number","Server Port",MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        EnableWindow(Start,1);
        return; 
  }                              
  //reset servers
  EnableWindow(S[0],0); EnableWindow(S[1],0); EnableWindow(S[2],0); EnableWindow(S[3],0);   
  SetWindowText(ExtPortStart,""); SetWindowText(ExtPortEnd,"");
  SetWindowText(IntPortStart,""); SetWindowText(IntPortEnd,"");
  SetWindowText(ToIP,"");  SetWindowText(Name,""); SetWindowText(Version,"");
  SetWindowText(World,"");  SetWindowText(Mode,"");      
  //create socket for both server and client, same socket different ports
  ServerSock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  if(ServerSock==INVALID_SOCKET)
  { 
      MessageBox(hWnd, "Cant't init DHCP socket","Error",MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
      EnableWindow(Start,1);
      return;
  }
  setsockopt(ServerSock, SOL_SOCKET, SO_REUSEADDR,(const char *)&True, sizeof(True));
  setsockopt(ServerSock, SOL_SOCKET, SO_BROADCAST,(const char *)&True, sizeof(True));

  //fill server structure for listening socket
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;  
  server.sin_port=htons(port); 
  //fill client structure for sending packets
  client.sin_family = AF_INET;
  client.sin_port = htons(port);
  client.sin_addr.s_addr = inet_addr("255.255.255.255");
     
  //bind the socket
  if(SendMessage(Bind,BM_GETCHECK,0,0)==BST_CHECKED)
  {
  	    ret = bind(ServerSock, (struct sockaddr*)&server, sizeof(server));
        if(ret!=0)
        { 
             MessageBox(hWnd, "Cant't bind to socket","Error",MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
             closesocket(ServerSock);
             EnableWindow(Start,1);
             return;
        }
  }
  //send PING
  sprintf(PACKED_UNCONNECTED_PING.time,"%08X",GetTickCount());
  p=(UCHAR*)&PACKED_UNCONNECTED_PING;
  n = sizeof(PACKED_UNCONNECTED_PING);
  //send 2 pings to be sure
 //ret = sendto(ServerSock,p,n,0,(struct sockaddr*)&client, sizeof(client));
  ret = sendto(ServerSock,p,n,0,(struct sockaddr*)&client, sizeof(client));
  if(ret <= 0)
  {
       MessageBox(hWnd, "Send Ping FAILED!!!","Error",MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
       closesocket(ServerSock);
       EnableWindow(Start,1);
       return; 
  }
  else
  {
      UCHAR data[1024];
      SOCKADDR_IN    from;
      n=0; //server count
      int from_len = sizeof(from);          
      do
      {
          memset(data,0,1024);
          ret = net_isrecv(ServerSock,1);
          if(ret!=1) break;
          ret = recvfrom(ServerSock,data, 1024,0,(struct sockaddr*)&from,&from_len);
          if(ret>0)
          {
              if(data[0] != 0x1c)  //if is not the right code response
                  continue;
              data[ret]=0;
              if(DecodePacket(&data[sizeof(RESPONCE_UNCONNECTED_PONG)], ret,&D[n],inet_ntoa(from.sin_addr))==1)
              {
                     EnableWindow(S[n],1); n++;
                     if(n==4) break;
              }
          }
          else
          {
              break;
          }
     }while(ret>0);
  }
  closesocket(ServerSock);
  EnableWindow(Start,1);
}

void ShowConfig(int n)
{
     UCHAR temp[128];
     //extport
     SetWindowText(ExtPortStart,PORT);
     SetWindowText(ExtPortEnd,PORT);
     //intport
     SetWindowText(IntPortStart,D[n].Port);
     SetWindowText(IntPortEnd,D[n].Port);
     //ip
     SetWindowText(ToIP,D[n].DATA);
     //info
     SetWindowText(Name,D[n].PlayerName);
     SetWindowText(Version,D[n].Version);
     SetWindowText(World,D[n].WorldName);
     sprintf(temp,"%s    %s / %s Players",D[n].GameMode,D[n].cPlayers,D[n].mPlayers);
     //SetWindowText(Mode,D[n].GameMode);
     SetWindowText(Mode,temp);
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
 switch (message)
 {
  case WM_CREATE:
  {
    hWnd=hwnd;
	CreateWindow("BUTTON","",WS_VISIBLE|WS_CHILD|BS_GROUPBOX,4,-4,186,36,hwnd,(HMENU)0,ins,NULL);
	PingPort=CreateWindow("EDIT","19132",WS_CHILD|WS_VISIBLE|ES_NUMBER,10,9,47,17,hwnd,(HMENU)0,ins,NULL);
	Bind=CreateWindow("BUTTON","bind",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_TEXT,63,9,57,17,hwnd,(HMENU)0,ins,NULL);
	Start=CreateWindow("BUTTON","Ping",WS_CHILD|WS_VISIBLE,124,8,62,20,hwnd,(HMENU)ID_PING,ins,NULL);
    SendMessage(PingPort,EM_SETLIMITTEXT,(WPARAM)0x5,(LPARAM)0); 

	CreateWindow("BUTTON","ExtPort",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,4,38,152,65,hwnd,(HMENU)0,ins,NULL);
	
    CreateWindow("BUTTON","Start",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,14,58,62,37,hwnd,(HMENU)0,ins,NULL);
	ExtPortStart=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,21,76,47,15,hwnd,(HMENU)0,ins,NULL);

	CreateWindow("BUTTON","End",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,84,58,62,37,hwnd,(HMENU)0,ins,NULL);
	ExtPortEnd=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,91,76,47,15,hwnd,(HMENU)0,ins,NULL);

	CreateWindow("BUTTON","IntPort",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,164,38,152,65,hwnd,(HMENU)0,ins,NULL);
	CreateWindow("BUTTON","Start",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,174,58,62,37,hwnd,(HMENU)0,ins,NULL);
	IntPortStart=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,181,76,47,15,hwnd,(HMENU)0,ins,NULL);

	CreateWindow("BUTTON","End",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,244,58,62,37,hwnd,(HMENU)0,ins,NULL);
	IntPortEnd=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,251,76,47,15,hwnd,(HMENU)0,ins,NULL);

	CreateWindow("BUTTON","IP",WS_VISIBLE|WS_CHILD|BS_GROUPBOX|BS_CENTER,324,38,129,65,hwnd,(HMENU)0,ins,NULL);
	ToIP=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,331,76,115,15,hwnd,(HMENU)0,ins,NULL);
	
    CreateWindow("STATIC","NAME:",WS_CHILD|WS_VISIBLE,4,116,65,15,hwnd,(HMENU)0,ins,NULL);
    Name=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,77,116,375,15,hwnd,(HMENU)0,ins,NULL);
    CreateWindow("STATIC","VERSION:",WS_CHILD|WS_VISIBLE,4,136,65,15,hwnd,(HMENU)0,ins,NULL);
    Version=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,77,136,375,15,hwnd,(HMENU)0,ins,NULL);
    CreateWindow("STATIC","WORLD:",WS_CHILD|WS_VISIBLE,4,156,65,15,hwnd,(HMENU)0,ins,NULL);
    World=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,77,156,375,15,hwnd,(HMENU)0,ins,NULL);
    CreateWindow("STATIC","MODE:",WS_CHILD|WS_VISIBLE,4,176,65,15,hwnd,(HMENU)0,ins,NULL);
    Mode=CreateWindow("EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,77,176,375,15,hwnd,(HMENU)0,ins,NULL);

	S[0]=CreateWindow("BUTTON","Server1",WS_CHILD|WS_VISIBLE|WS_DISABLED,194,8,60,20,hwnd,(HMENU)ID_SERVER_1,ins,NULL);
	S[1]=CreateWindow("BUTTON","Server2",WS_CHILD|WS_VISIBLE|WS_DISABLED,260,8,60,20,hwnd,(HMENU)ID_SERVER_2,ins,NULL);
	S[2]=CreateWindow("BUTTON","Server3",WS_CHILD|WS_VISIBLE|WS_DISABLED,326,8,60,20,hwnd,(HMENU)ID_SERVER_3,ins,NULL);
	S[3]=CreateWindow("BUTTON","Server4",WS_CHILD|WS_VISIBLE|WS_DISABLED,392,8,60,20,hwnd,(HMENU)ID_SERVER_4,ins,NULL);

    CenterOnScreen();
    WSAStartup(MAKEWORD(2,0),&wsaData);
	
  }
  break;
    
  case WM_COMMAND:
       switch(LOWORD(wParam))
       {
           case ID_PING:
           { 
              CreateThread(0,0,(LPTHREAD_START_ROUTINE)Ping,0,0,0); 
           } break; 
           case ID_SERVER_1:
           {
                ShowConfig(0);
           } break;
           case ID_SERVER_2:
           {
                ShowConfig(1);
           } break;
           case ID_SERVER_3:
           {
                ShowConfig(2);
           } break;
           case ID_SERVER_4:
           {
                ShowConfig(3);
           } break;
//-----------------------------------------------------------------------------                                           
       }
       break;  
        case WM_DESTROY:
        {
             WSACleanup();
            PostQuitMessage (0);
        }
            break;
        default: 
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}


int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE a,LPSTR b, int nFunsterStil)
{
    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl;

    ins = hThisInstance;
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof (WNDCLASSEX);
    wincl.hIcon = LoadIcon (ins,MAKEINTRESOURCE(200));
    wincl.hIconSm = LoadIcon (ins,MAKEINTRESOURCE(200));
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND+1;

    if (!RegisterClassEx (&wincl))
        return 0;
    hwnd = CreateWindowEx(0,szClassName,"MCPE-Server Config",WS_SYSMENU|WS_MINIMIZEBOX,
           0,0,462,230,HWND_DESKTOP,NULL,hThisInstance,NULL);
    ShowWindow (hwnd, nFunsterStil);
    while (GetMessage (&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
 return messages.wParam;
}

void CenterOnScreen()
{
     RECT rcClient, rcDesktop;
	 int nX, nY;
     SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);
     GetWindowRect(hWnd, &rcClient);
	 nX=((rcDesktop.right - rcDesktop.left) / 2) -((rcClient.right - rcClient.left) / 2);
	 nY=((rcDesktop.bottom - rcDesktop.top) / 2) -((rcClient.bottom - rcClient.top) / 2);
SetWindowPos(hWnd, NULL, nX, nY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

