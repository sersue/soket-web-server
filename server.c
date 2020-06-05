#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/types.h> // definitions of a number of data types used in socket.h and netinet/in.h
#include <strings.h>
#define BUFFER_SIZE 256
#define BACKLOG 20 //최대 20명

//error처리
void error(char *msg){
    perror(msg);
    exit(1);
} 

//파일요청과 그에대한 응답 구현
char *filerequest(char *path){

    char *types[] = { ".html", ".jpeg",".gif", ".mp3","pdf" };//요청할 파일 종류들
        char *content_type[] = { "text/html", "image/jpeg", "image/gif", "audio/mp3", "application/pdf" };
        int len = (int)(sizeof(types) / sizeof(types[0])); //type의 길이

    char *result = content_type[0];
    
    for(int i=0; i<len; i++){
        //받은 path와 type[]을 각각 비교
        if(strstr(path, types[i]) != NULL){
            result = (char *) malloc(strlen(content_type[i]) + 1);
            //result에 content_type copy
            strcpy(result, content_type[i]);
            break;
        }
    }
    return result;
} 

int main(int argc, char **argv){
    int sockfd, newsockfd;
    int portno; //port number
    char buffer[BUFFER_SIZE];
    char buffer1[BUFFER_SIZE]; 
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;


    //error처리
    if(argc < 2){
        fprintf(stderr,"ERROR\n");
        exit(1);
    }
    /* socket() */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        error("ERROR");
    }

    //같은 포트 번호 썼을 때 안들어가져서 so_reuseaddr 추가
    int re;
    re = 1;
    setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&re, sizeof(re) );
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* bind() */
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR on binding");
    }
    /*server_addr ->sockaddr 타입으로 변환 
    ip랑 port값을 sockfd에 바인딩*/

    /* listen()*/
    listen(sockfd, BACKLOG);
    clilen = sizeof(cli_addr);

    while(1){
        //newsockfd에 accept의 결과 저장
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); 

        if(newsockfd < 0){
            error("ERROR on accept");
        }
        //buffer 비우기
        bzero(buffer,BUFFER_SIZE);//read하기위해 buffer초기화 (해달라는거)
        bzero(buffer1,BUFFER_SIZE);// 받은 메시지 담는 곳 (해주는거)
        //read()
        if(read(newsockfd, buffer, BUFFER_SIZE) < 0){
            error("ERROR reading from socket");
        }
        printf("%s\n", buffer);
        //strtok 문자열 잘라서 비교
        char *tm = strtok(buffer, " "); 
        //buffer가 null이 아니면
        if (tm != NULL){

            char *rawpath = strtok(NULL, " ");
            printf("%s\n", rawpath);

            if ( rawpath != NULL ) {
                //path 생성
                char *path;
                //strcmp s1,s2 문자열 비교 
                if(strcmp(rawpath, "/") == 0){ 
                    path = (char *)malloc(strlen("./index.html") + 1);
                    //문자열을 path 에 복사 
                    strcpy(path, "./index.html");
                }
                else{
                    path = (char *)malloc(strlen(rawpath) + 1);
                    sprintf(path, ".%s", rawpath);
                }
                //위에 만들어놓은 함수에 path 전달 
                char *content_type = filerequest(path); 

                printf("%s\n", content_type);
                printf("%s\n", path);
                
                //file open
                int file = open(path, O_RDONLY);
                
                //Path를 읽은 후 response에 파일 사이즈 lseek로 측정
                int fileSize = lseek(file, 0, SEEK_END);
                lseek(file, 0, SEEK_SET);
                sprintf(buffer1, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: Alive\r\n\r\n", content_type, fileSize);
                printf("%s\n\n",buffer1);
                //write()
                if (write(newsockfd, buffer1, strlen(buffer1))< 0) error("ERROR");

                while ((fileSize = read(file, buffer, BUFFER_SIZE)) > 0){
                    if (write(newsockfd, buffer, BUFFER_SIZE) < 0) error("ERROR");
                }
                //close()
                close(file);
                   
            } 
        } 
    } 
    //close()
    close(newsockfd);
    close(sockfd);

    return 0;
} 