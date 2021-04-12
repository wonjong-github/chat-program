#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<sys/select.h>

#define BUF_SIZE 1024
void error_handling(char *message);
void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf, char *name);

int main(int argc, char *argv[]){
  int sock;   //소켓용 변수 선언.
  char buf[BUF_SIZE];
  struct sockaddr_in serv_adr;  //연결요청할 서버의 주소정보를 저장할 변수
  fd_set reads, cpy_reads;    //멀티플렉싱을 하기 위한 fd_set 선언, 원본과 복사본을 따로 선언해 관찰할 fd가 변화되지 않도록 한다.
  int fd_max, i, fd_num;  //검사대상 fd의 개수, select함수를 통해 변화가 생긴 fd의 개수를 저장할 변수
  struct timeval timeout;   //select함수 실행시 blocking을 방지하기 위한 timeout을 지정할 변수 선언.

  if(argc != 4){    //이름까지 입력받아야 하므로 인자가 4개가 아니면 종료
    printf("Usage : %s <IP> <port> <name>\n", argv[0]);
    exit(1);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0);   //ipv4, tcp 서버 소켓생성
  if(sock == -1)    //소켓 생성 오류시
    error_handling("socket() error");

  memset(&serv_adr, 0, sizeof(serv_adr));   //연결할 서버정보 구조체 초기화
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));   //연결요청할 서버의 정보 입력

  if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)  //연결요청
    error_handling("connect() error!");

  FD_ZERO(&reads);    //원본 fd_set변수 초기화
  FD_SET(sock, &reads);   //클라이언트 소켓을 fd_set에 등록. 수신할 데이터가 있는지 감지.
  FD_SET(0, &reads);      //표준입력으로부터 변화가 있는지 감지하기 위해 등록.
  // FD_SET(1, &reads);
  fd_max = sock;    //현재 클라이언트 소켓이 fd값이 가장 크므로 그것을 최대 개수로 설정.
  while(1){   //while문으로 개속 감지.
    cpy_reads = reads;    //카피본 생성해서 원본을 살려둔다.
    timeout.tv_sec = 5;
    timeout.tv_usec = 5000;   //timeout을 while문안에 선언해서 select함수 실행전 초기화를 시켜준다.

    //2. select함수는 항상 복사본을 넣어야한다.
    if((fd_num = select(fd_max+1, &cpy_reads, 0, 0, &timeout))==-1)
      break;    //select함수 실패시
    if(fd_num == 0)
      continue;   //변화가 있는 fd가 없을 경우

      //변화가 있는 fd가 있다면 실행
    for(i=0; i<fd_max+1; i++){    //어떤 fd에 변화가 있는지 하나씩 확인
      if(FD_ISSET(i, &cpy_reads)){    //만일 그 fd가 변화가 있다면
        if(i==0){   //표준 입력이 들어 온경우
          write_routine(sock, buf, argv[3]);  //write_routine함수 실행
        }else if(i==sock){    //클라이언트 소켓에 수신할 데이터가 존재하는가
          read_routine(sock, buf);    //write_routine함수 실행
        }
      }
    }
  }
  close(sock);
  return 0;
}
void read_routine(int sock, char *buf){   //두개의 프로세스를 따로 실행하는 것이 아니므로 while문을 지웠다.
  int str_len=read(sock, buf, BUF_SIZE);   //데이터를 읽어온다.
  if(str_len==0)    //그 값이 없다면 종료.
    return ;
  buf[str_len] = 0;
  printf("%s", buf);  //있다면 출력
}

void write_routine(int sock, char *buf, char *name){
  char total_msg[BUF_SIZE+20];  //전체 보낼 메세지 변수 선언. 전체 메세지 + 보내는 사람 이름
  fgets(buf, BUF_SIZE, stdin);  //입력된 문자열을 읽어온다.
  if(!strcmp(buf,"q\n") || !strcmp(buf, "Q\n")){  //만일 그것이 종료 요청이면
    shutdown(sock, SHUT_WR);    //half-close실행
    return ;
  }
  sprintf(total_msg, "[%s] : %s", name, buf);   //보내는 사람 + 메세지 형태의 문자열을 만든다.
  write(sock, total_msg, strlen(total_msg));  //그것을 소켓에 보낸다.
}
void error_handling(char *message){
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
