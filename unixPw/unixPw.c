/*
* unixPw.c
* 
* Coded by 1ndr4 (indra.kr@gmail.com)
* 
* https://github.com/indra-kr/Tools/blob/master/unixPw/unixPw.c
* 
* Compile: cc -o unixPw unixPw.c -lcrypt
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <shadow.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define _XOPEN_SOURCE

#define SCALE	(24L*3600L)

extern char *crypt(const char *key, const char *salt);
void Encrypt(const char *pw, int mode);
void Compare(const char *pw, const char *epw);

int main(int argc, char **argv)
{
	int opt = 0, type = 0, crypto = 4;
	char *pw = NULL, *epw = NULL;

	if(argc < 4) {
		fprintf(stdout, 
			"Usage: %s -t <Type> -c <Cryptography> -p <Plaintext Password>\n"
			"       %s -t <Type> -e <Encrypted Password> -p <Plaintext Password>\n\n"
			"[: TYPES :]\n"
			" 1: Creating a encrypted password\n"
			" 2: Comparing a password\n\n"
			"[: CRYPTOGRAPHY :]\n"
			" 1: MD5 SHADOW\n"
			" 2: SHA-256 SHADOW\n"
			" 3: SHA-512 SHADOW\n"
			" 4: DES (DEFAULT)\n", argv[0], argv[0]);
		return 0;
	}

	while((opt = getopt(argc, argv, "t:c:p:e:")) != -1) {
		switch(opt) {
		case 't':
			type = atoi(optarg);
			break;
		case 'c':
			crypto = atoi(optarg);
			break;
		case 'p':
			pw = optarg;
			break;
		case 'e':
			epw = optarg;
			break;
		default:
			fprintf(stderr, "[!] Illegal option\n");
			return -1;
		}
	}
	switch(type) {
	case 1:
		fprintf(stdout, "[*] Creating a encrypted password\n");
		Encrypt(pw, crypto);
		break;
	case 2:
		fprintf(stdout, "[*] Comparing a password\n");
		Compare(pw, epw);
		break;
	default:
		fprintf(stdout, "[!] Unknown type\n");
		return -1;
	}
	return 0;
}

void Encrypt(const char *pw, int mode)
{	
	char key[40] = {0,};
	char *cipher = NULL;
	struct timeval tv;

	if(pw == NULL) {
		fprintf(stderr, "[!] Insufficient information\n");
		return;
	}

	memset(key, 0x00, sizeof(key));
	memcpy(key, "$1$", 3);
	gettimeofday(&tv, (struct timezone *)0);
	strcat(key, l64a(tv.tv_usec));
	strcat(key, l64a(tv.tv_sec));
	strcat(key, l64a(getpid() + clock()));
	strcat(key, l64a(clock()));
	strcat(key, l64a(tv.tv_usec + getpid()));
	
	fprintf(stdout, "[*] Mode: ");
	switch(mode) {
	case 1:
		fprintf(stdout, "MD5\n");
		key[11] = '\0';
		break;
	case 2:
		fprintf(stdout, "SHA-256\n");
		key[1] = '5';
		key[19] = '\0';
		break;
	case 3:
		fprintf(stdout, "SHA-512\n");
		key[1] = '6';
		key[19] = '\0';
		break;
	case 4:
	default:
		fprintf(stdout, "DES (Default)\n");
		memcpy(key, l64a(tv.tv_usec), 2);
		break;
	}

	if((cipher = (char*)crypt(pw, key)) == NULL) {
		fprintf(stdout, "[!] crypt() function  error.\n");
		return;
	}
	fprintf(stdout, " [>] Plaintext Password: %s\n"
			" [>] Encrypted Password: %s\n"
			"[*] Done.\n" , pw, cipher);	
	return;
}

void Compare(const char *pw, const char *epw)
{
	char key[40] = {0,};
	char *cipher = NULL;

	if(pw == NULL || epw == NULL) {
		fprintf(stderr, "[!] Insufficient information\n");
		return;
	}
	memset(key, 0x00, sizeof(key));

	fprintf(stdout, "[*] Type: ");
	if(memcmp(epw, "$1$", 3) == 0) {
		fprintf(stdout, "MD5\n");
		memcpy(key, epw, 11);
	} else if(memcmp(epw, "$5$", 3) == 0) {
		fprintf(stdout, "SHA-256\n");
		memcpy(key, epw, 19);
	} else if(memcmp(epw, "$6$", 3) == 0) {
		fprintf(stdout, "SHA-512\n");
		memcpy(key, epw, 19);
	} else {
		fprintf(stdout, "DES\n");
		memcpy(key, epw, 2);
	}
	if((cipher = (char*)crypt(pw, key)) == NULL) {
		fprintf(stdout, "[!] crypt() function error.\n");
		return;
	}
	fprintf(stdout, " [>] User's plaintext password: %s\n"
			" [>] User's encrypted password: %s\n"
			" [>] Encrypted Password by user's plaintext password: %s\n", pw, epw, cipher);

	if(strcmp(cipher, epw) == 0) {
		fprintf(stdout,	"[*] Authentication is OK!\n");
	} else {
		fprintf(stdout, "[!] Authentication is fail!\n");
			
	}
	return;
}
