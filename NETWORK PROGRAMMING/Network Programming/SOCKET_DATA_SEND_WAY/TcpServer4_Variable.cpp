//tcp 가변길이 전송 서버
#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

//소켓 수신 버퍼에서 데이터를 한꺼번에 읽어 내부에 저장해두고 읽기 요청이 있을 때마다 1Byte씩 리턴 함수
int recv_ahead(SOCKET s, char *p){
	__declspec(thread) static int nbytes = 0;//__declspec(thread)는 멀티 스레드용 선언
	__declspec(thread) static char buf[1024];
	__declspec(thread) static char *ptr;

	if(nbytes==0 || nbytes==SOCKET_ERROR){
		nbytes = recv(s, buf, sizeof(buf), 0);
		if(nbytes==SOCKET_ERROR) return SOCKET_ERROR;
		else if(nbytes==0) return 0;
		ptr = buf;
	}
	--nbytes;
	*p = *ptr++;
	return 1;
}

//\n이 나올 때나 최대 길이까지 읽는 함수
int recvline(SOCKET s, char *buf, int maxlen){
	int n, nbytes;
	char c;
	char *ptr = buf;

	for(n=1; n<maxlen; n++){
		nbytes = recv_ahead(s, &c);
		if(nbytes==1){
			*ptr++ = c;
			if(c=='\n') break;
		}
		else if(nbytes==0){
			*ptr = 0;
			return n-1;
		}
		else return SOCKET_ERROR;
	}
	*ptr = 0;
	return n;
}

int main(int argc, char* argv[]) {
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	//소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	//bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	//listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	//데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	while (1) {
		//accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		//접속한 클라이언트 정보
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("TCP accept client: ip=%s, port=%d\n", addr, ntohs(clientaddr.sin_port));

		//클라이언트와 데이터 통신
		while (1) {
			//데이터 받기
			retval = recvline(client_sock, buf, BUFSIZE+1);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0) break;

			//받은 데이터 출력
			printf("[TCP|%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);
		}

		//소켓 닫기
		closesocket(client_sock);
		printf("TCP close client: ip=%s, port=%d\n", addr, ntohs(clientaddr.sin_port));
	}

	//소켓 닫기
	closesocket(listen_sock);

	//윈속 종료
	WSACleanup();
	return 0;
}