#include <iostream>
#ifndef WIN32
   #include <arpa/inet.h>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <chrono>
#include <srt.h>

#include "skrewt_util.h"
#include "node_api.h"

// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

using namespace std;

#ifndef WIN32
void* sendfile(void*);
#else
DWORD WINAPI sendfile(LPVOID);
#endif

napi_value testMethod(napi_env env, napi_callback_info info) {
  napi_status status;

  int start_state = srt_startup();

  SRTSOCKET sock = srt_socket(AF_INET, SOCK_DGRAM, 0);
  int close_state = srt_close(sock);

  // int end_state = srt_cleanup();

  napi_value result;
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  printf("Guess who got called! %i %i\n", start_state, close_state);

  return result;
}

napi_value receiveFile(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value result;
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  size_t argc = 4;
  napi_value args[4];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  char nodeName[256];
  char serviceName[256];
  char requestName[256];
  char storeName[256];
  status = napi_get_value_string_latin1(env, args[0], nodeName, 256,  nullptr);
  CHECK_STATUS;
  status = napi_get_value_string_latin1(env, args[1], serviceName, 256, nullptr);
  CHECK_STATUS;
  status = napi_get_value_string_latin1(env, args[2], requestName, 256, nullptr);
  CHECK_STATUS;
  status = napi_get_value_string_latin1(env, args[3], storeName, 256, nullptr);
  CHECK_STATUS;

  // use this function to initialize the UDT library
  srt_startup();

  srt_setloglevel(logging::LogLevel::debug);

  struct addrinfo hints, *peer;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  SRTSOCKET fhandle = srt_socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
  // SRT requires that third argument is always SOCK_DGRAM. The Stream API is set by an option,
  // although there's also lots of other options to be set, for which there's a convenience option,
  // SRTO_TRANSTYPE.
  SRT_TRANSTYPE tt = SRTT_FILE;
  srt_setsockopt(fhandle, 0, SRTO_TRANSTYPE, &tt, sizeof tt);

  if (0 != getaddrinfo(nodeName, serviceName, &hints, &peer))
  {
     cout << "incorrect server/peer address. " << nodeName << ":" << serviceName << endl;
     return result;
  }

  cout << "wibble: " << peer->ai_addr << endl;

  // connect to the server, implict bind
  if (SRT_ERROR == srt_connect(fhandle, peer->ai_addr, peer->ai_addrlen))
  {
     cout << "connect: " << srt_getlasterror_str() << endl;
     return result;
  }

  freeaddrinfo(peer);

  // send name information of the requested file
  int len = strlen(requestName);

  if (SRT_ERROR == srt_send(fhandle, (char*)&len, sizeof(int)))
  {
     cout << "send: " << srt_getlasterror_str() << endl;
     return result;
  }

  if (SRT_ERROR == srt_send(fhandle, requestName, len))
  {
     cout << "send: " << srt_getlasterror_str() << endl;
     return result;
  }

  // get size information
  int64_t size;

  if (SRT_ERROR == srt_recv(fhandle, (char*)&size, sizeof(int64_t)))
  {
     cout << "send: " << srt_getlasterror_str() << endl;
     return result;
  }

  if (size < 0)
  {
     cout << "no such file " << requestName << " on the server\n";
     return result;
  }

  // receive the file
  //fstream ofs(argv[4], ios::out | ios::binary | ios::trunc);
  int64_t recvsize;
  int64_t offset = 0;

  SRT_TRACEBSTATS trace;
  srt_bstats(fhandle, &trace, true);

  if (SRT_ERROR == (recvsize = srt_recvfile(fhandle, storeName, &offset, size, SRT_DEFAULT_RECVFILE_BLOCK)))
  {
     cout << "recvfile: " << srt_getlasterror_str() << endl;
     return result;
  }

  srt_bstats(fhandle, &trace, true);

  cout << "speed = " << trace.mbpsRecvRate << "Mbits/sec" << endl;
  int losspercent = 100*trace.pktRcvLossTotal/trace.pktRecv;
  cout << "loss = " << trace.pktRcvLossTotal << "pkt (" << losspercent << "%)\n";

  srt_close(fhandle);

  //ofs.close();

  // use this function to release the UDT library
  srt_cleanup();

  return result;
}

napi_value srtSendFile(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value result;
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  char service[256];
  status = napi_get_value_string_latin1(env, args[0], service, 256, nullptr);
  CHECK_STATUS;

  // use this function to initialize the UDT library
  srt_startup();

  srt_setloglevel(logging::LogLevel::debug);

  addrinfo hints;
  addrinfo* res;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  if (0 != getaddrinfo(NULL, service, &hints, &res))
  {
     cout << "illegal port number or port is busy.\n" << endl;
     return 0;
  }

  SRTSOCKET serv = srt_socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  // SRT requires that third argument is always SOCK_DGRAM. The Stream API is set by an option,
  // although there's also lots of other options to be set, for which there's a convenience option,
  // SRTO_TRANSTYPE.
  SRT_TRANSTYPE tt = SRTT_FILE;
  srt_setsockopt(serv, 0, SRTO_TRANSTYPE, &tt, sizeof tt);

  // Windows UDP issue
  // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
  int mss = 1052;
  srt_setsockopt(serv, 0, SRTO_MSS, &mss, sizeof(int));
#endif

  //int64_t maxbw = 5000000;
  //srt_setsockopt(serv, 0, SRTO_MAXBW, &maxbw, sizeof maxbw);

  if (SRT_ERROR == srt_bind(serv, res->ai_addr, res->ai_addrlen))
  {
     cout << "bind: " << srt_getlasterror_str() << endl;
     return 0;
  }

  freeaddrinfo(res);

  cout << "server is ready at port: " << service << endl;

  srt_listen(serv, 10);

  sockaddr_storage clientaddr;
  int addrlen = sizeof(clientaddr);

  SRTSOCKET fhandle;

  while (true)
  {
     if (SRT_INVALID_SOCK == (fhandle = srt_accept(serv, (sockaddr*)&clientaddr, &addrlen)))
     {
        cout << "accept: " << srt_getlasterror_str() << endl;
        return 0;
     }

     char clienthost[NI_MAXHOST];
     char clientservice[NI_MAXSERV];
     getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
     cout << "new connection: " << clienthost << ":" << clientservice << endl;

     #ifndef WIN32
        pthread_t filethread;
        pthread_create(&filethread, NULL, sendfile, new SRTSOCKET(fhandle));
        pthread_detach(filethread);
     #else
        CreateThread(NULL, 0, sendfile, new SRTSOCKET(fhandle), 0, NULL);
     #endif
  }

  srt_close(serv);

  // use this function to release the UDT library
  srt_cleanup();

  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("test", testMethod),
    DECLARE_NAPI_METHOD("receive", receiveFile),
    DECLARE_NAPI_METHOD("send", srtSendFile)
   };
  status = napi_define_properties(env, exports, 3, desc);
  CHECK_STATUS;

  return exports;
}

NAPI_MODULE(nodencl, Init)

#ifndef WIN32
void* sendfile(void* usocket)
#else
DWORD WINAPI sendfile(LPVOID usocket)
#endif
{
   SRTSOCKET fhandle = *(SRTSOCKET*)usocket;
   delete (SRTSOCKET*)usocket;

   // aquiring file name information from client
   char file[1024];
   int len;

   if (SRT_ERROR == srt_recv(fhandle, (char*)&len, sizeof(int)))
   {
      cout << "recv: " << srt_getlasterror_str() << endl;
      return 0;
   }

   if (SRT_ERROR == srt_recv(fhandle, file, len))
   {
      cout << "recv: " << srt_getlasterror_str() << endl;
      return 0;
   }
   file[len] = '\0';

   // open the file (only to check the size)
   fstream ifs(file, ios::in | ios::binary);

   ifs.seekg(0, ios::end);
   int64_t size = ifs.tellg();
   //ifs.seekg(0, ios::beg);
   ifs.close();

   // send file size information
   if (SRT_ERROR == srt_send(fhandle, (char*)&size, sizeof(int64_t)))
   {
      cout << "send: " << srt_getlasterror_str() << endl;
      return 0;
   }

   SRT_TRACEBSTATS trace;
   srt_bstats(fhandle, &trace, true);

   // send the file
   int64_t offset = 0;
   if (SRT_ERROR == srt_sendfile(fhandle, file, &offset, size, SRT_DEFAULT_SENDFILE_BLOCK))
   {
      cout << "sendfile: " << srt_getlasterror_str() << endl;
      return 0;
   }

   srt_bstats(fhandle, &trace, true);
   cout << "speed = " << trace.mbpsSendRate << "Mbits/sec" << endl;
   int losspercent = 100*trace.pktSndLossTotal/trace.pktSent;
   cout << "loss = " << trace.pktSndLossTotal << "pkt (" << losspercent << "%)\n";

   srt_close(fhandle);

   //ifs.close();

   #ifndef WIN32
      return NULL;
   #else
      return 0;
   #endif
}
