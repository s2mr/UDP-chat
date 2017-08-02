#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "MessagePacket.h"

#define ECHOMAX (255)			/* エコー文字列の最大長 */
#define TIMEOUT (2)				/* select関数のタイムアウト値 [秒] */

/* キーボードからの文字列入力・エコーサーバへの送信処理関数 */
int SendEchoMessage(int sock, struct sockaddr_in *pServAddr);

/* ソケットからのメッセージ受信・表示処理関数 */
int ReceiveEchoMessage(int sock, struct sockaddr_in *pServAddr);

int SendJoinRequest(int sock, struct sockaddr_in *pServAddr);

int main(int argc, char *argv[])
{
  char *servIP;					/* エコーサーバのIPアドレス */
  unsigned short servPort;		/* エコーサーバのポート番号 */
  
  int sock;						/* ソケットディスクリプタ */
  struct sockaddr_in servAddr;	/* エコーサーバ用アドレス構造体 */

  int maxDescriptor;			/* select関数が扱うディスクリプタの最大値 */
  fd_set fdSet;					/* select関数が扱うディスクリプタの集合 */
  struct timeval tout;			/* select関数におけるタイムアウト設定用構造体 */

  /* 引数の数を確認する．*/
  if ((argc < 2) || (argc > 3)) {
    fprintf(stderr,"Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
    exit(1);
  }
  
  /* 第1引数からエコーサーバのIPアドレスを取得する．*/
  servIP = argv[1];

  /* 第2引数からエコーサーバのポート番号を取得する．*/
  if (argc == 3) {
    /* 引数が在ればエコーサーバのポート番号として使用する．*/
    servPort = atoi(argv[2]);
  }
  else {
    /* 引数が無ければエコーサーバのポート番号として 7番 を使用する．*/
    /* (※ 7番 はエコーサービスの well-known ポート番号）*/
    servPort = 7;
  }

  /* メッセージの送受信に使うソケットを作成する．*/
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    fprintf(stderr, "socket() failed");
    exit(1);
  }
  
  /* エコーサーバ用アドレス構造体へ必要な値を格納する．*/
  memset(&servAddr, 0, sizeof(servAddr));			/* 構造体をゼロで初期化 */
  servAddr.sin_family		= AF_INET;				/* インターネットアドレスファミリ */
  servAddr.sin_addr.s_addr	= inet_addr(servIP);	/* サーバのIPアドレス */
  servAddr.sin_port			= htons(servPort);		/* サーバのポート番号 */

  /* select関数で処理するディスクリプタの最大値として，ソケットの値を設定する．*/
  maxDescriptor = sock;
  
  /* サーバに参加したいというリクエストを送信する */
  SendJoinRequest(sock, &servAddr);

  /* 文字列入力・メッセージ送信，およびメッセージ受信・表示処理ループ */
  for (;;) {

    /* ディスクリプタの集合を初期化して，キーボートとソケットを設定する．*/
    // 関数形式のマクロ
    FD_ZERO(&fdSet);				/* ゼロクリア */
    // STDIN_FILENO -> 0
    // FD_SET -> 該当するビットを１にする
    FD_SET(STDIN_FILENO, &fdSet);	/* キーボード(標準入力)用ディスクリプタを設定 */
    FD_SET(sock, &fdSet);			/* ソケットディスクリプタを設定 */

    /* タイムアウト値を設定する．*/
    tout.tv_sec  = TIMEOUT; /* 秒 */
    tout.tv_usec = 0;       /* マイクロ秒 */

    /* 各ディスクリプタに対する入力があるまでブロックする．*/
    // 3, 4番目ー＞標準出力、TCPの帯域外データ受信があるかどうかをチェックする
    if (select(maxDescriptor + 1, &fdSet, NULL, NULL, &tout) == 0) {
      /* タイムアウト */
      // printf(".\n");
      continue;
    }

    /* キーボードからの入力を確認する．*/
    if (FD_ISSET(STDIN_FILENO, &fdSet)) {
     /* キーボードからの入力があるので，文字列を読み込み，エコーサーバへ送信する．*/
      if (SendEchoMessage(sock, &servAddr) < 0) {
        break;
      }
    }
    
    /* ソケットからの入力を確認する．*/
    if (FD_ISSET(sock, &fdSet)) {
      /* ソケットからの入力があるので，メッセージを受信し，表示する．*/
      if (ReceiveEchoMessage(sock, &servAddr) < 0) {
        break;
      }
    }
  }

  /* ソケットを閉じ，プログラムを終了する．*/
  close(sock);
  exit(0);
}

int SendJoinRequest(int sock, struct sockaddr_in *pServAddr) {
  char pktBuf[ECHOMAX];
  int msgID = MSGID_JOIN_REQUEST;
  // printf("send-------------\n");
  int pktMsgLen = Packetize(msgID, NULL, 0, pktBuf, ECHOMAX);
  // printf("pktMsgLen: %d\n", pktMsgLen);
  /* エコーサーバへメッセージ(入力された文字列)を送信する．*/
  int sendMsgLen = sendto(sock, pktBuf, pktMsgLen, 0,
    (struct sockaddr*)pServAddr, sizeof(*pServAddr));
  // printf("sendMsgLen: %d\n", sendMsgLen);
    
  return sendMsgLen;
}

/*
 * キーボードからの文字列入力・エコーサーバへのメッセージ送信処理関数
 */
int SendEchoMessage(int sock, struct sockaddr_in *pServAddr)
{
  // printf("send-------------------\n");
  char echoString[ECHOMAX + 1];	/* エコーサーバへ送信する文字列 */
  int echoStringLen;			/* エコーサーバへ送信する文字列の長さ */
  int sendMsgLen;				/* 送信メッセージの長さ */

  /* キーボードからの入力を読み込む．(※改行コードも含まれる．) */
  if (fgets(echoString, ECHOMAX + 1, stdin) == NULL) {
    /*「Control + D」が入力された．またはエラー発生．*/
    return -1;
  }

  /* 入力文字列の長さを確認する．*/
  echoStringLen = strlen(echoString);
  if (echoStringLen < 1) {
    fprintf(stderr,"No input string.\n");
    return -1;
  }

  /* エコーサーバへメッセージ(入力された文字列)を送信する．*/
  char pktBuf[ECHOMAX];
  int pktMsgLen = Packetize(MSGID_CHAT_TEXT, echoString, echoStringLen, pktBuf, ECHOMAX);
  // printf("pktMsgLen : %d\n", pktMsgLen);
  // printf("echoString : %s\n", echoString);
  // printf("pktBuf : %s\n", pktBuf);
  
  sendMsgLen = sendto(sock, pktBuf, pktMsgLen, 0,
    (struct sockaddr*)pServAddr, sizeof(*pServAddr));
    

  /* 送信されるべき文字列の長さと送信されたメッセージの長さが等しいことを確認する．*/
  if (pktMsgLen != sendMsgLen) {
    fprintf(stderr, "sendto() sent a different number of bytes than expected");
    return -1;
  }
  return 0;
}

/*
 * ソケットからのメッセージ受信・表示処理関数(パケットが到着した後に呼ばれる)
 */
int ReceiveEchoMessage(int sock, struct sockaddr_in *pServAddr)
{
  // printf("received--------------------\n");

  struct sockaddr_in fromAddr;	/* メッセージ送信元用アドレス構造体 */
  unsigned int fromAddrLen;		/* メッセージ送信元用アドレス構造体の長さ */
  char recvPktBuffer[ECHOMAX + 1];	/* パケット受信バッファ */
  char recvMsgBuffer[ECHOMAX + 1];	/* パケット受信バッファ */
  int recvPktLen;				/* 受信パケットの長さ */
  int recvMsgLen;       /* 受信メッセージの長さ */
  
  memset(recvMsgBuffer, '\0', ECHOMAX+1);

  /* エコーメッセージ送信元用アドレス構造体の長さを初期化する．*/
  fromAddrLen = sizeof(fromAddr);

  /* エコーメッセージを受信する．*/
  recvPktLen = recvfrom(sock, recvPktBuffer, ECHOMAX, 0,
    (struct sockaddr*)&fromAddr, &fromAddrLen);
  if (recvPktLen < 0) {
    fprintf(stderr, "recvfrom() failed");
    return -1;
  }

  /* エコーメッセージの送信元がエコーサーバであることを確認する．*/
  if (fromAddr.sin_addr.s_addr != pServAddr->sin_addr.s_addr) {
    fprintf(stderr,"Error: received a packet from unknown source.\n");
    return -1;
  }
  
  short msgID = MSGID_NONE;
  short msgBufSize;
  memcpy(&msgBufSize, &recvPktBuffer[2], sizeof(short));
  
  static short myID;
  short userID=0;
  memcpy(&userID, &recvPktBuffer[4], sizeof(short));
  // printf("msgBufSize: %d\n", msgBufSize);
  // printf("msgID: %x\n", msgID);
  // printf("recvPktLen: %d\n", recvPktLen);
  // printf("recvMsgBuffer: %s\n", recvMsgBuffer);
  // printf("recvPktBuffer: %s\n", recvPktBuffer);
  // printf("recvMsgLen: %d\n", recvMsgLen);

  recvMsgLen = Depacketize(recvPktBuffer, recvPktLen, &msgID, recvMsgBuffer, msgBufSize);
  
  static int isFirst=1;
  switch(msgID) {
    case MSGID_JOIN_RESPONSE:
      printf("ルームに入室しました\n");
      // printf("recvMsgBuffer: %s\n", recvMsgBuffer);
      // printf("最初のuserID: %d\n", userID);
      if(isFirst) {
        isFirst = 0;
        myID = userID;
      }
      break;
    case MSGID_CHAT_TEXT:
      if(myID==userID)
        printf("ME: %s\n", recvMsgBuffer);
      else
        // printf("My ID: %d\n", myID);
        // printf("userID: %d\n", userID);
        printf("Client %d: %s\n",userID, recvMsgBuffer);
      break;
    default:
      printf("終了します（しません）\n");
      break;
  }
  
  return 0;
}

int Packetize(short msgID, char *msgBuf, short msgLen, char *pktBuf, int pktBufSize) {
  // printf("msgID: %x\n", msgID);
  
  memcpy(&pktBuf[0], &msgID, sizeof(short));
  memcpy(&pktBuf[2], &msgLen, sizeof(short));
  memcpy(&pktBuf[4], msgBuf, msgLen);
  
  return 2 + 2 + msgLen;
}

int Depacketize(char *pktBuf, int pktLen, short *msgID, char *msgBuf, short msgBufSize) {
  memcpy(msgID, &pktBuf[0], sizeof(short));
  memcpy(msgBuf, &pktBuf[6], msgBufSize-sizeof(short));
  
  return strlen(msgBuf);
}
