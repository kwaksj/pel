#include <stdio.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/pem.h>

#define IN_FILE  "encrypt.bin"
#define OUT_FILE  "plain.txt"

unsigned char * readFile(char * file,int *readLen);
unsigned char * addString(unsigned char *destString,int destLen,const unsigned char *addString,int addLen);

int main()
{
	// 키와 IV값은 직접 만듬	
	unsigned char key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	unsigned char iv[] = {1,2,3,4,5,6,7,8};

	BIO *errBIO = NULL;
	BIO *outBIO = NULL;
	
	// 에러 발생의 경우 해당 에러 스트링 출력을 위해 미리 에러 스트링들을 로딩.
	ERR_load_crypto_strings(); 
	
	// 표준 화면 출력 BIO 생성
	if ((errBIO=BIO_new(BIO_s_file())) != NULL)
		BIO_set_fp(errBIO,stderr,BIO_NOCLOSE|BIO_FP_TEXT);
	// 파일 출력 BIO 생성
	outBIO = BIO_new_file(OUT_FILE,"wb");
	
	if (!outBIO)	
	{ // 에러가 발생한 경우
		BIO_printf(errBIO,"파일 [%s] 을 생성 하는데 에러가 발생 했습니다.",OUT_FILE);
		ERR_print_errors(errBIO);
		exit(1);
	}
	
	// 파일에서 읽는다
	int len;
	unsigned char * readBuffer = readFile(IN_FILE,&len);
	
	// 암호화 컨텍스트 EVP_CIPHER_CTX 생성,초기화	
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);

	// 초기화 
	EVP_DecryptInit_ex(&ctx, EVP_bf_cbc(), NULL, key, iv);
	
	// 초기화가 끝난후에 해야 한다. 복호문 저장할 버퍼 생성
	unsigned char * outbuf = (unsigned char *)malloc(sizeof(unsigned char) *  len);
	int outlen, tmplen;

	//업데이트, 마지막 블록을 제외 하고 모두 복호화
	if(!EVP_DecryptUpdate(&ctx, outbuf, &outlen, readBuffer, len))	
	{		
		return 0;
	}

	// 종료. 마지막 블록을 복호화	
	if(!EVP_DecryptFinal_ex(&ctx, outbuf + outlen, &tmplen))
	{
		return 0;	
	}

	// 복호문 길이는 업데이트, 종료 과정에서 나온 결과의 합
	outlen += tmplen;
	EVP_CIPHER_CTX_cleanup(&ctx);
	
	BIO_printf(errBIO,"복호화가 완료 되었습니다. 복호문은 다음과 같습니다.\n\n");
	outbuf[outlen] = 0;	
	printf("%s",outbuf);
	
	// 파일에 같은 내용을 출력 한다
	BIO_write(outBIO, outbuf, outlen);
	
	// 객체 제거
	BIO_free(outBIO);
	return 0;
} 

unsigned char * readFile(char * file,int *readLen)
{
	unsigned char * retBuffer = NULL;
	unsigned char * buffer = NULL;
	int length = 0;

	// 파일 BIO 정의	
	BIO *fileBIO = NULL;

	// 인자로 넘어온 파일을 열고, 파일 BIO 생성	
	fileBIO = BIO_new_file(file,"rb");
	if (!fileBIO)
	{ // 파일을 여는데 에러가 발생한 경우
		printf("입력 파일 [%s] 을 여는데 에러가 발생 했습니다.",file);
		exit(1);	
	}
	
	// 임시로 1000바이트 만큼의 읽은 데이터를 저장할 버퍼 생성	
	buffer = (unsigned char *)malloc(1001);	
	*readLen = 0;
	
	while(1)
	{
		// 파일 BIO에서 1000 바이트 만큼 읽어서 buffer에 저장 한다.
		length = BIO_read(fileBIO,buffer,1000);		
		// 안전을 위해 버퍼의 끝은 NULL로 채운다.
		buffer[length] = 0;
		// 임시로 읽은 1000바이트의 데이터 리턴 버퍼에 더한다.
		retBuffer = addString(retBuffer,*readLen,buffer,length);
		// 지금 까지 읽은 데이터의 길이를 더한다.
		*readLen = *readLen + length;
		
		// 만약 지금 파일에서 읽은 데이터의 길이가 꼭 1000바이트 라면 앞으로 더 읽을		
		// 데이터가 있을 것이다. 하지만 1000 바이트 보다 작다면 더 이상 읽을 데이터가	
		// 없을 것이므로 종료 한다.
		if (length == 1000)
			// 파일 포인터를 1000바이트 뒤로 옮긴다.			
			BIO_seek(fileBIO,1000);     
		else
			break;	
}
	// 객체 삭제	
	BIO_free(fileBIO);
	free(buffer);

	return retBuffer;
}

unsigned char *addString(unsigned char *destString,int destLen,const unsigned char *addString,int addLen)
{
	// 리턴 할 버퍼 정의
	unsigned char * retString;
	// 만약 덧붙일 대상 버퍼가 NULL, 이거나 길이가 0이면 덧붙일 대상버퍼가 없는 경우 
	// 이므로 새로 생성 하고, 덧붙일 버퍼의 내용을 복사 한다.
	int i;

	if ((destString == NULL) || (destLen==0))		
	{
		// 덧붙일 버퍼의 길이 만큼의 버퍼 생성
		retString = (unsigned char * )malloc(sizeof(unsigned char) * (addLen+1));
		// 덧붙일 버퍼의 내용을 새로운 버퍼에 복사
		for (i=0;i<addLen;i++)
		{	
			retString[i] = addString[i];	
		}

		// 안전을 위해 버퍼의 마지막에 NULL 바이트를 붙인다.
		(&retString)[i] = NULL;
		// 덧붙일 대상 버퍼가 있는 경우 이므로 덧붙일 대상 버퍼의 길이와 덧붙일 버퍼의 길이를
		// 더한 만큼의 버퍼를 새로 생성하고, 두 버퍼의 내용을 새로운 버퍼에 복사 한다.	
	}
	else
	{	
		// 대상 버퍼의 길이와 덧붙일 버퍼의 길이를 더한 만큼의 버퍼 생성
		retString = (unsigned char * )malloc(sizeof(unsigned char) * (destLen+addLen+1));
		// 덧붙일 대상 버퍼 내용을 새로운 버퍼에 복사		
		for (i=0;i<destLen;i++)
		{
			retString[i] = destString[i];	
		}
		// 덧붙일 버퍼의 내용을 새로운 버퍼에 복사
		for (i=0;i< addLen;i++)
		{
			retString[i+destLen] = addString[i];	
		}	
		// 안전을 위해 버퍼의 마지막에 NULL 바이트를 붙인다.
		(&retString)[i+destLen] = NULL;		
	}
	
	// 메모리에서 삭제
	free(destString);
	
	return retString;
}



