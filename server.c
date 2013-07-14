#include "network.h"
void send_recv_loop(int acc);
/*サーバソケットの準備*/
int server_socket(const char *portnm)
{
	char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct addrinfo hints, *res0;
	int soc, opt, errcode;
	socklen_t opt_len;

	/*アドレス情報のヒントをゼロクリア*/
	(void) memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	/*アドレス情報の決定*/
	if ((errcode = getaddrinfo(NULL, portnm, &hints, &res0)) != 0){
		(void) fprintf(stderr, "getaddrinfo():%s\n",gai_strerror(errcode));
		return (-1);
	}
	if((errcode = getnameinfo(res0->ai_addr, res0->ai_addrlen,
							  nbuf, sizeof(nbuf),
							  sbuf, sizeof(sbuf),
							  NI_NUMERICHOST | NI_NUMERICSERV)) != 0){
		freeaddrinfo(res0);
		return (-1);
	}
	(void) fprintf(stderr, "port=%s\n", sbuf);
	
	/*ソケットの生成*/
	if ((soc = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol)) == -1 ){
		perror("socket");
		freeaddrinfo(res0);
		return (-1);
	}
	
	/*ソケットオプション(再利用フラグ)設定*/
	opt = 1;
	opt_len = sizeof(opt);
	if (setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len) == -1){
		perror("setsockopt");
		(void) close(soc);
		freeaddrinfo(res0);
		return (-1);
	}
	
	/*ソケットにアドレスを指定*/
	if (bind(soc, res0->ai_addr, res0->ai_addrlen) == -1){
		perror("bind");
		(void) close(soc);
		freeaddrinfo(res0);
		return (-1);
	}
	
	/*アクセスバックログの指定*/
	if (listen(soc, SOMAXCONN) == -1){
		perror("listen");
		(void) close(soc);
		freeaddrinfo(res0);
		return (-1);
	}
	
	freeaddrinfo(res0);
	return (soc);
}

void accept_loop( int soc )
{
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct sockaddr_storage from;
	int acc;
	socklen_t len;
	for(;;){
		len = (socklen_t) sizeof(from);
		/*接続受付*/
		if ((acc = accept(soc, (struct sockaddr *) &from, &len)) == -1){
			if( errno != EINTR ){
				perror("accept");
			}
		} else {
			(void) getnameinfo((struct sockaddr *) &from, len,
							   hbuf, sizeof(hbuf),
							   sbuf, sizeof(sbuf),
							   NI_NUMERICHOST | NI_NUMERICSERV);
			(void) fprintf(stderr, "accept+%s+%s\n", hbuf, sbuf);
			/*送受信ループ*/
			send_recv_loop(acc);
			/*アクセプトソケットクローズ*/
			(void) close(acc);
			acc = 0;
		}
	}
}

/*サイズ指定文字列連結*/
size_t mystrlcat(char *dst, const char *src, size_t size)
{
	const char *ps;
	char *pd, *pde;
	size_t dlen, lest;

	for(pd = dst, lest = size; *pd != '\0' && lest != 0; pd++, lest--);
	dlen = pd - dst;
	if (size - dlen == 0){
		return (dlen + strlen(src));
	}

	pde = dst + size -1;
	for(ps = src; *ps != '\0' && pd < pde; pd++, ps++){
		*pd = *ps;
	}
	for(; pd < pde; pd++){
		*pd = '\0';
	}
	while(*ps++);
	return (dlen + (ps - src -1));
}

/*送受信ループ*/
void send_recv_loop(int acc)
{
	FILE *fp;
	char buf[512], fileName[512] = "", *ptr, *file;
	long fileSize;
	ssize_t len;
	int fileNameSize = 0;
	for(;;){
		/*受信*/
		if((len = recv(acc, buf, sizeof(buf), 0)) == -1){
			/*エラー*/
			perror("recv");
			break;
		}
		if(len == 0){
			/*EOF*/
			(void) fprintf(stderr, "recv:EOF\n");
			break;
		}
		/*文字列化・表示*/
		buf[len]='\0';
		if((ptr = strpbrk(buf, "\r\n")) != NULL){
			*ptr = '\0';
		}
		(void) fprintf(stderr, "[client]%s\n", buf);
		/*応答文字列作成*/
		if( (ptr = strstr( buf, "GET" )) != NULL ){
			/*fileSeverMode*/
			ptr += strlen("GET") + 1;
			fileNameSize = 0;
			if(*ptr == '"'){
				ptr++;
				while(*(ptr + fileNameSize) != '"')
					fileNameSize++;
			}
			strncpy(fileName, ptr, fileNameSize);
			if((fp = fopen(fileName, "rb")) == NULL)
				(void) fprintf(stderr, "ファイルオープンに失敗\n");
			else{
				(void) fseek( fp, 0, SEEK_END );
				fileSize = ftell(fp);
				if( (file = malloc(sizeof(char) * fileSize)) == NULL )
					(void) fprintf(stderr, "メモリ確保に失敗\n");
				(void) fseek(fp, 0, SEEK_SET);
				(void) fread(file, sizeof(char), fileSize, fp);
			fclose(fp);
			}
			/*応答*/
			snprintf(buf, sizeof(buf)
					 ,"fileNameSize:%dbyte\nfileName:%s\nfileSize:%ldbyte\n"
					 ,fileNameSize, fileName, fileSize);
			len = (ssize_t) strlen(buf);
			if(( len = send(acc, buf, (size_t) len, 0)) == -1){
				/*エラー*/
				perror("send");
				break;
			}
			len = (ssize_t) strlen(file);
			if((len = send(acc, file, (size_t) len, 0)) == -1){
				/*エラー*/
				perror("send");
				break;
			}
		}
		else{
			/*echoServerMode*/
				len = (ssize_t) strlen(buf);
				/*応答*/
				if((len = send(acc, buf, (size_t) len, 0)) == -1){
					/*エラー*/
					perror("send");
					break;
				}
		}
	}
}

int main(int argc, char* argv[])
{
	int soc;
	/*引数にポート番号が指定されているか？*/
	if(argc <= 1){
		(void) fprintf(stderr, "server port\n");
		return (EX_USAGE);
	}
	if((soc = server_socket(argv[1])) == -1){
		(void) fprintf(stderr, "server_socket(%s):error\n",argv[1]);
		return (EX_UNAVAILABLE);
	}
	(void) fprintf(stderr, "ready for accept\n");
	/*アクセプトループ*/
	accept_loop(soc);
	/*ソケットクローズ*/
	(void) close(soc);
	return (EX_OK);
}
