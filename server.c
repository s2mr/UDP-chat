/* 
 * ■情報ネットワーク実践論（橋本担当分）  サンプルソースコード (6)
 *
 * UDPエコーサーバー：非同期I/O対応版 ※レポート課題2※
 *
 *	コンパイル：gcc -Wall -o UDPEchoServer-AsyncRep UDPEchoServer-AsyncRep.c
 *		使い方：UDPEchoServer-Async <Echo Port>
 *
 *		※利用するポート番号は 10000 + 学籍番号の下4桁
 *			［例］学籍番号が 0312013180 の場合： 10000 + 3180 = 13180
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <signal.h>

#define ECHOMAX (255)				/* エコー文字列の最大長 */

int sock;							/* ソケットディスクリプタ */
void IOSignalHandler(int signo);	/* SIGIO 発生時のシグナルハンドラ */

int main(int argc, char *argv[])
{
  unsigned short servPort;			/* エコーサーバ(ローカル)のポート番号 */
  struct sockaddr_in servAddr;		/* エコーサーバ(ローカル)用アドレス構造体 */
  struct sigaction sigAction;		/* シグナルハンドラ設定用構造体 */

  /* 引数の数を確認する．*/
  if (argc != 2) {
    fprintf(stderr,"Usage: %s <Echo Port>\n", argv[0]);
    exit(1);
  }
  /* 第1引数からエコーサーバ(ローカル)のポート番号を取得する．*/
  servPort = atoi(argv[1]);

  /* メッセージの送受信に使うソケットを作成する．*/
  //■未実装■  
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sock < 0) {
    fprintf(stderr, "socket() failed");
    exit(1);
  }

  /* エコーサーバ(ローカル)用アドレス構造体へ必要な値を格納する．*/
  //■未実装■  
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  /* ソケットとエコーサーバ(ローカル)用アドレス構造体を結び付ける．*/
  //■未実装■  
  if(bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
    fprintf(stderr, "bind() failed");
    exit(1);
  }

  /* シグナルハンドラを設定する．*/
  //TImeoutを発生させてください  -> SIGALRM
  //TimeoutしたらHander()という関数を呼んでください
  //■未実装■
  sigAction.sa_handler = IOSignalHandler; //関数の先頭番地を代入

  /* ハンドラ内でブロックするシグナルを設定する(全てのシグナルをブロックする)．*/
  //■未実装■
  if(sigfillset(&sigAction.sa_mask) < 0) {
    fprintf(stderr, "sigfillset() failed\n");
    exit(1);
  }

  /* シグナルハンドラに関するオプション指定は無し．*/
  // ■未実装■
  sigAction.sa_flags = 0;

  /* シグナルハンドラ設定用構造体を使って，シグナルハンドラを登録する．*/
  //■未実装■
  if (sigaction(SIGIO, &sigAction, 0) < 0) {
    fprintf(stderr, "sigaction() failed\n");
    exit(1);
  }

  /* このプロセスがソケットに関するシグナルを受け取るための設定を行う．*/
  //パケットが到着したらシグナルを受け取る
  //fcntl -> ファイルの設定
  if (fcntl(sock, F_SETOWN, getpid()) < 0) {
    fprintf(stderr, "Unable to set process owner to us\n");
    exit(1);
  }

  /* ソケットに対してノンブロッキングと非同期I/Oの設定を行う．*/
  //ノンブロッキング＝即時復帰
  //recvfromでパケットの受信がなくてもブロックしないようにする
  //
  if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
    fprintf(stderr, "Unable to put the sock into nonblocking/async mode\n");
    exit(1);
  }

  /* エコーメッセージの受信と送信以外の処理をする．*/
  for (;;) {
    printf(".\n");
    sleep(2);
  }

  /* ※このエコーサーバプログラムは，この部分には到達しない */
}

/* SIGIO 発生時のシグナルハンドラ */
//パケットが到着した時に呼ばれる
void IOSignalHandler(int signo)
{
  struct sockaddr_in clntAddr;	/* クライアント用アドレス構造体 */
  unsigned int clntAddrLen;		/* クライアント用アドレス構造体の長さ */
  char msgBuffer[ECHOMAX];		/* メッセージ送受信バッファ */
  int recvMsgLen;				/* 受信メッセージの長さ */
  int sendMsgLen;				/* 送信メッセージの長さ */

  /* 受信データがなくなるまで，受信と送信を繰り返す．*/
  do {
    /* クライアント用アドレス構造体の長さを初期化する．*/
    //■未実装■
    clntAddrLen = sizeof(clntAddr);

    /* クライアントからメッセージを受信する．(※この呼び出しはブロックしない) */
    // ■未実装■
    recvMsgLen = recvfrom(sock, msgBuffer, ECHOMAX, 0, (struct sockaddr*)&clntAddr,
     &clntAddrLen);

    /* 受信メッセージの長さを確認する．*/
    if (recvMsgLen < 0) {
      /* errono が EWOULDBLOCK である場合，受信データが無くなったことを示す．*/
      /* EWOULDBLOCK は，許容できる唯一のエラー．*/
      if (errno != EWOULDBLOCK) {
        fprintf(stderr, "recvfrom() failed\n");
        exit(1);
      }
    } else {
      /* クライアントのIPアドレスを表示する．*/
      // ■未実装■
      printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));

      /* 受信メッセージをそのままクライアントに送信する．*/
      // ■未実装■
      sendMsgLen = sendto(sock, msgBuffer, recvMsgLen, 0, (struct sockaddr*)&clntAddr,
       sizeof(clntAddr));

      /* 受信メッセージの長さと送信されたメッセージの長さが等しいことを確認する．*/
      // ■未実装■
      if(recvMsgLen != sendMsgLen) {
        fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
        exit(1);
      }
    }
  } while (recvMsgLen >= 0);
}