#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/select.h>

#define MAXBUFLEN 256
#define DOCBUFLEN 10000

//struct to hold result after processing response
typedef struct ResponseResult
{
  int maxLen;
  int responseCode;
  char request[MAXBUFLEN];
  char response[MAXBUFLEN];
  char filePath[MAXBUFLEN];

} ResponseResult;

//to ensure user can't use "/../../" to traverse directories in reverse
int backDirCheck(char string[]){

  int var = 0;
  char dot[1] = ".";
  int i = 0;
  for(i; i<strlen(string); i++){

      if(i != (strlen(string)-1)){
        if(string[i] == dot[0]){
          if(string[i+1] == dot[0]){
            var = 1;
            // printf("\n%d\n", var);
          }
        }
      }
  }

  // printf("%d\n", var );
  return var;
}

//logs response to server after request has been processed
void logResponse(ResponseResult * rr, const char ip[], int portno, char fileAddr[]){

    char res[50];

    switch(rr->responseCode){

      case 200:
        strcpy(res,"HTTP/1.0 200 OK\0");
        strcpy(rr->response,"HTTP/1.0 200 OK\n\n");
        // printf("in200\n");
        break;

      case 400:
        strcpy(res,"HTTP/1.0 400 Bad Request\0");
        strcpy(rr->response,"HTTP/1.0 400 Bad Request\n");
        break;

      case 404:
        strcpy(res,"HTTP/1.0 404 Not Found\0");
        strcpy(rr->response,"HTTP/1.0 404 Not Found\n");
        // printf("in404\n");
        break;

      default:
        strcpy(res,"Error with response code.\0");
        strcpy(rr->response,"Error with response code.\n");
        break;
    }

    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(buffer,80, "%b %d %T",timeinfo);

    printf("%s %s:%d %s; %s; %s\n", buffer, ip, portno, rr->request, res, fileAddr);
}

//turns string into lowercase
//used in getResponse() to verify requests
char* reqCheck(char req[]){

    // printf("here");
    int i = 0;
    while(req[i]){
        req[i] = tolower(req[i]);
        i++;
    }

    return req;
}

//reads a file into doc and returns
//last filled index of doc
int readFile(FILE* file, char* doc[DOCBUFLEN]){

    // printf("here\n" );
    char line[DOCBUFLEN];
    int i = 0;

    while(fgets(line, sizeof(line), file) != NULL){
        if(i>DOCBUFLEN-1){
            break;
        }
        strcpy(doc[i],line);
        i++;
    }

    return i;
}

//processes request sent in by user and sets
//up the ResponseResult typedef
void getResponse(char req[], char ** document, ResponseResult * rs, char fileAddr[]){

    char* arguments[3] = {NULL};
    char* token;
    int count = 0;
    FILE* file = NULL;

    //ensure response full of text has been provided
    if((token = strtok (req," \r\n")) == NULL){
      rs->responseCode = 400;
      return;
    }

    //tokenize and differentiate http verb, file, protocol
    while (count < 3){
        if(token != NULL){
          arguments[count] = token;
          strcat(rs->request, token);

          if(count !=2){
          strcat(rs->request, " ");
          }
        }
        // printf("%s\n",token);
        token = strtok (NULL, " \r\n");
        count++;
    }

    // printf("%s    %s    %s\n", arguments[0], arguments[1], arguments[2]);
    //Check for valid request method
    if(arguments[0] != NULL){
      if(strcmp(reqCheck(arguments[0]),"get") != 0){
        rs->responseCode = 400;
        if(arguments[1] != NULL){
          strcat(fileAddr,arguments[1]);
        }
  // printf("1\n");
        return;
      }
    } else{
      rs->responseCode = 400;
      // printf("here\n");
      return;
    }

    //Check for valid protocol/version
    // printf("ja");

    if(arguments[2]!=NULL){
      if(strcmp(reqCheck(arguments[2]), "http/1.0") != 0){
        rs->responseCode = 400;
        if(arguments[1]!=NULL){
            strcat(fileAddr,arguments[1]);
         }
        return;
      }
    } else{
      rs->responseCode = 400;
      // printf("here2\n" );
      if(arguments[1]!=NULL){
        strcat(fileAddr,arguments[1]);
      }
      return;
    }

    //fix file path
    if(arguments[1]==NULL){
      rs->responseCode = 400;
      // printf("here3\n" );
      return;
    }

    int var = strcmp(arguments[1],"/");
    char slash[1] = "/";

    switch(var){

      case 0:
        strcat(fileAddr,"/index.html");
        break;

      default:
        if(arguments[1][0] != slash[0]){
          rs->responseCode = 400;
          // printf("22\n");
          strcat(fileAddr,arguments[1]);
          return;
        }

        else{
          strcat(fileAddr,arguments[1]);
          break;
        }
    }

    // printf("%s\n",fileAddr);

    if(backDirCheck(fileAddr) == 1){
        rs->responseCode = 404;
        strcpy(rs->filePath,fileAddr);

        // printf("4\n");
        return;
    }

    if((file = fopen(fileAddr,"r")) == NULL){
        rs->responseCode = 404;
        strcpy(rs->filePath,fileAddr);

        // printf("5\n");
        return;
    } else{
        // read file
        // return max line number in file
        int maxLen = readFile(file, document);
        fclose(file);
        rs->responseCode = 200;
        strcpy(rs->filePath, fileAddr);
        rs->maxLen = maxLen;
        return;
    }
}

//check for request from client or q as keyboard input,
//send for processing accordingly
int checkReadyToRead(int sockfd){
  fd_set readfds;
  struct timeval tv;
  FD_ZERO(&readfds);
  FD_SET( STDIN_FILENO, &readfds);
  FD_SET(sockfd, &readfds);

  tv.tv_sec = 10;
  tv.tv_usec = 500000;

  int ret = select((sockfd+1), &readfds, NULL, NULL, &tv);

  switch(ret){
    case 0:
      return 0;

    case -1:
      printf("Error in checkReadyToRead\n");
      return -1;

    default:
      if( FD_ISSET(STDIN_FILENO, &readfds)){

        char buff[10];
        scanf("%s",buff);
        if(strcmp(buff,"q")==0){
          printf("Server shutting down.\n");
          exit(1);
        }
      }

      else if(FD_ISSET(sockfd, &readfds)){
        return 2;
      }
  }
}

int main(int argc, char *argv[]){

    // printf("%d\n",argc );
    if (argc < 3) {
      printf( "Usage: %s <port> <directory>\n", argv[0] );
      return -1;
    }

    //Using socket() function for communication
    //int socket(int domain, int type, int protocol);
    //domain is PF_INET, type is SOCK_DGRAM for UDP, and protocol can be set to 0 to choose the proper protocol!
    int sockfd, portno;
    char dir[50];
    socklen_t cli_len;
    char buffer[MAXBUFLEN]; //data buffer
    struct sockaddr_in serv_addr, cli_addr; //we need these structures to store socket info
    int numbytes;

    //The first step: creating a socket of type of UDP
    //error checking for every function call is necessary!
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
     perror("sws: error on socket()");
     return -1;
     }

    /* prepare the socket address information for server side:
    (IPv4 only--see struct sockaddr_in6 for IPv6)

    struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
    };
    */

    strcpy(dir,argv[2]);

    //Clear the used structure by using either memset():
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);  //read the port number value from stdin

    serv_addr.sin_family = AF_INET; //Address family, for us always: AF_INET

    const char * ip = "10.10.1.100";
    serv_addr.sin_addr.s_addr = inet_addr(ip); //INADDR_ANY; //Listen on any ip address I have
    serv_addr.sin_port = htons(portno);  //host to network byte order

    //Bind the socket with the address information:
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
      close(sockfd);
      perror("sws: error on binding!");
      return -1;
    }

    printf("sws is running on UDP Port %d and serving %s\npress 'q' to quit ...\n", portno, dir);

    while(1){

      int ret = checkReadyToRead(sockfd);

      if(ret == 2){

        char** doc = (char**)malloc(DOCBUFLEN * sizeof(char*));
        long j = 0;
        for( j; j<DOCBUFLEN; j++ ) {
          doc[j] = (char*)malloc(DOCBUFLEN);
        }

        cli_len = sizeof(cli_addr);

        //the sender address information goes to cli_addr
        if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN-1 , 0,(struct sockaddr *)&cli_addr, &cli_len)) == -1) {
          perror("sws: error on recvfrom()!");
          return -1;
        }

        buffer[numbytes] = '\0';
        ResponseResult *result = malloc(sizeof(ResponseResult));
        strcpy(result->request,"");

        char fileAddr[50] = ".";
        strcat(fileAddr,dir);

        getResponse(buffer, doc, result, fileAddr);
        logResponse(result, ip, ntohs(cli_addr.sin_port),fileAddr);

        //create buffer to be sent
        char **bufferToBeSent = (char**) malloc(result->maxLen * sizeof(char *));
        int count = 0;
        for(count; count < result->maxLen; count++){
            bufferToBeSent[count] = (char *) malloc(sizeof(char) * DOCBUFLEN);
            strcpy(bufferToBeSent[count],doc[count]);
        }

        int count3 = 0;
        if ((numbytes = sendto(sockfd, result->response, strlen(result->response), 0,(struct sockaddr *)&cli_addr, cli_len)) == -1) {
                    perror("sws: error in sendto()");
                    return -1;
                }

        while(count3<result->maxLen){
          if ((numbytes = sendto(sockfd, bufferToBeSent[count3], strlen(bufferToBeSent[count3]), 0,(struct sockaddr *)&cli_addr, cli_len)) == -1) {
              perror("sws: error in sendto()");
              return -1;
          }

          if(count3 == (result->maxLen-1)){
            if ((numbytes = sendto(sockfd, "\n", strlen("\n"), 0,(struct sockaddr *)&cli_addr, cli_len)) == -1) {
                perror("sws: error in sendto()");
                return -1;
            }
          }
          count3++;
        }

        free(doc);
        free(bufferToBeSent);
        free(result);
      }

    }

    close(sockfd);
    return 0;
}
