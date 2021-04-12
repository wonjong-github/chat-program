#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<sys/select.h>
#include<time.h>

#define BUF_SIZE 100
void error_handling(char *buf);
struct clnt_info{ //한 클라이언트의 정보를 저장하는 구조체정의
  char clnt_ip[20];
  int clnt_port;
};

int main(int argc, char *argv[]){
  int serv_sock, clnt_sock;   //서버, 클라이언트 소켓 정의
  struct sockaddr_in serv_adr, clnt_adr;
  struct timeval timeout;   //select함수 실행시 timeout을 지정할 변수 선언
  fd_set reads, cpy_reads;  //여러개의 file descriptor을 핸들링하기 위해 fd_set변수 선언, 초기 설정값을 기억하기위해 초기 설정과 카피본 두개를 선언
  struct tm *now;     //now, ltime -> asctime을 사용하기 위한 변수 선언.
  time_t ltime;

  socklen_t adr_sz;
  int fd_max, str_len, fd_num, i, j; //검사대상 fd의 개수. 클라이언트로부터 읽어들인 문자열 개수, select함수를 통해 등록된 파일디스크립터중 변화가 생긴 개수.
  char buf[BUF_SIZE];
  if(argc != 2){
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }   //초기 실행시 인자가 2개가 아니면 종료.

  serv_sock = socket(PF_INET, SOCK_STREAM, 0);    //ipv4, tcp 서버 소켓생성
  memset(&serv_adr, 0, sizeof(serv_adr));   //서버소켓 정보 저장할 구조체 초기화
  serv_adr.sin_family = AF_INET;      //tcp사용
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);     //로컬 ip로 설정
  serv_adr.sin_port = htons(atoi(argv[1]));   //입력받은 인자로 포트 설정

  if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)  //서버 주소, 포트 번호 할당.
    error_handling("bind() error");
  if(listen(serv_sock, 5) == -1)    //연결요청 대기.
    error_handling("listen() error");

  FD_ZERO(&reads);      //fd_set을 0으로 초기화.
  FD_SET(serv_sock, &reads);    //서버소켓을 fd_set에 등록
  fd_max = serv_sock;     //fd의 개수를 서버소켓의 fd값으로 설정(현재 서버소켓이 fd값이 가장 크다).
  struct clnt_info clnt_info_arr[10];   //연결요청하는 클라이언트의 정보를 저장할 구조체를 배열의 형태로 선언.
  while(1){
    cpy_reads = reads;    //select함수를 실행하면 fd_set의 값이 달라지므로 복사본을 만들어서 넣어주어야한다.
    timeout.tv_sec = 5;
    timeout.tv_usec = 5000;   //timeout 설정. while문안에 넣어서 select함수를 실행하기전에 항상 초기화 시켜주어야 한다.

    //2. select함수는 항상 복사본을 넣어야한다.
    if((fd_num = select(fd_max+1, &cpy_reads, 0, 0, &timeout))==-1)
      break;    //select함수 실패시 while문 종료
    if(fd_num ==0)   //변화가 감지된 fd가 없을 경우
      continue;

      //변화가 있는 fd가 있으면 실행
    for(i=0; i<fd_max+1; i++){    //무엇이 변화가 생겼는지 알기위해 하나씩 확인
      if(FD_ISSET(i, &cpy_reads)){      //만일 그 fd_set이 변화가 있다면
        if(i==serv_sock){   //그 fd가 서버 소켓이라면 연결요청.
          adr_sz = sizeof(clnt_adr);
          clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
          //클라이언트로부터 요청된 연결을 수락하고, 연결이 되면 연결된 소켓의 정보는 clnt_adr에 담겨진다.
          FD_SET(clnt_sock, &reads);    //원본 fd_set에 요청한 클라이언트를 추가해준다.
          char *clnt_ip_p = inet_ntoa(clnt_adr.sin_addr);   //inet_ntoa함수를 이용해 네트워크 바이트 순서로 정렬된 정수형 ip주소를 보기편하게 변환
          strcpy(clnt_info_arr[clnt_sock-4].clnt_ip, clnt_ip_p);     //ip정보를 클라이언트 정보 구조체에 저장.
          clnt_info_arr[clnt_sock-4].clnt_port = ntohs(clnt_adr.sin_port);    //port정보 저장.
          if(fd_max < clnt_sock)
            fd_max=clnt_sock;   //만일 fd_max가 새로 들어오는 소켓번호보다 작으면 늘려준다.
          printf("connected client : %d\n", clnt_sock);
          // printf("Client's IP : %s\n", clnt_info_arr[clnt_sock-4].clnt_ip);
          // printf("Client's Port : %d\n", clnt_info_arr[clnt_sock-4].clnt_port);
        }
        else{   //연결요청이 아니면 client로부터 데이터가 넘어온것
          str_len = read(i, buf, BUF_SIZE);   //클라이언트에서 보낸 데이터를 읽어온다.
          if(str_len==0){   //만일 그 데이터가 없다면
            FD_CLR(i, &reads);  //필요없으므로  fd_set에서 삭제
            close(i);   //fd 닫기
            printf("closed client: %d \n", i);
          }
          else{
            time(&ltime);
            now = localtime(&ltime);    //클라이언트의 데이터가 넘어온 시간 설정
            printf("---------------------------------------------------------\n");
            printf("Client's IP : %s\n", clnt_info_arr[i-4].clnt_ip);   //변화가 생긴 클라이언트의 ip출력
            printf("Client's Port : %d\n", clnt_info_arr[i-4].clnt_port); //변화가 생긴 클라이언트의 port출력
            buf[str_len] = 0;
            printf("%s", asctime(now));   //클라이언트의 데이터가 넘어온 시간 출력
            printf("%s", buf);    //클라이언트 메세지 출력
            printf("---------------------------------------------------------\n");
            for(j = 0; j<fd_max+1; j++){
              if(FD_ISSET(j, &reads)){
                if(!(j==serv_sock) && !(j==i)){     //서버도, 데이터를 보낸 클라이언트도 아닌 클라이언트들에게 전부 메세지 전송.
                    write(j, buf, str_len);
                }
              }
            }
          }
        }
      }
    }
  }
  close(serv_sock);
  return 0;
}
void error_handling(char *buf){
  fputs(buf, stderr);
  fputc('\n', stderr);
  exit(1);
}
