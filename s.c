#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char *argv[]) 
{ 
	struct sockaddr_in servaddr, cliaddr; 
	int serv_sock, clnt_sock;
	socklen_t cliaddrlen = sizeof(cliaddr);
	char buf[BUF_SIZE]; 
	char buf2[BUF_SIZE];
	char cli_ip[20]; 
	char filename[20]; 
	char temp[20];
	char w[] = "gcc ";
	char a[] = "a.out";
	int filesize=0; 
	int total=0, sread, fp;
	int ret, readb;
	FILE *fp2;

	if(argc != 2) { 
		printf("usage: %s port ", argv[0]); 
		exit(0); 
	} 
	
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
		error_handling("socket() error");

	memset((char *)&servaddr, 0, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
			error_handling("bind() error");

	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	while(1) 
	{ 
		puts("서버가 연결요청을 기다림.."); 
		clnt_sock = accept(serv_sock, (struct sockaddr *)&cliaddr, &cliaddrlen);

		if (clnt_sock == -1)
			error_handling("accept() error");
		else
			puts("클라이언트가 연결됨.."); 

		inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cli_ip, sizeof(cli_ip)); 
		printf( "IP : %s ", cli_ip ); 
		printf( "Port : %x \n", ntohs( cliaddr.sin_port) );

		memset( filename, 0, 20 ); 
		recv(clnt_sock, filename, sizeof(filename), 0 ); 
		printf( "%s \n", filename );

		strcpy(temp, filename);

		read(clnt_sock, &filesize, sizeof(filesize) ); 

		fp = open( filename, O_WRONLY | O_CREAT | O_TRUNC);

		printf( "file is receiving now.. \n" ); 
		while( total != filesize ) 
		{ 
			sread = recv(clnt_sock, buf, BUF_SIZE, 0);
			total += sread; 
			buf[sread] = 0; 
			write( fp, buf, sread );
		} 
		printf( "file translating is completed\n " ); 

		close(fp);

		strcat(w, temp);
		ret = system(w);
		printf("exit status %d\n", ret);

		fp2 = fopen(a, "rb");

		total = 0;
		
		send(clnt_sock, a, sizeof(a), 0);
		fseek(fp2, 0, SEEK_END);
		filesize = ftell(fp2);
		
		send(clnt_sock, &filesize, sizeof(filesize), 0);
		fseek(fp2, 0, SEEK_SET);

		while(total != filesize)
		{
		    readb = fread(buf2, sizeof(char), BUF_SIZE, fp2);
			total += readb;
			buf2[readb] = 0;
			send(clnt_sock, buf2, readb, 0);
		}
		
		printf("file translating is completed\n");

		fclose(fp2);
		close(clnt_sock); 
	}

	close(serv_sock);

	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

