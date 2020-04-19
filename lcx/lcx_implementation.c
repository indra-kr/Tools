#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

/**********************************************************************
 *                             COMMON
 *********************************************************************/
#define F_LOGW		0x0001
#define F_HEXW		0x0002
#define F_QUIET		0x0004

#define MAX_CHILD_NUM	16

/**********************************************************************
 *                             WINDOWS
 *********************************************************************/
#ifdef _WIN32
#include <malloc.h>
#include <windows.h>
#include <process.h>
#include <io.h>
#include <conio.h>
#include <winsock.h>
#include <lm.h>
#include <Shlwapi.h>

#pragma comment (lib, "ws2_32")
#pragma comment (lib, "netapi32.lib")
#pragma comment (lib, "Shlwapi.lib")
#pragma comment (lib, "Advapi32.lib")
#pragma comment (lib, "advapi32")

#define open            _open
#define read            _read
#define write           _write
#define lseek           _lseek

#define unlink(f)	_unlink(f);
#define sleep(s)        _sleep(s*1000)
#define close(s)        \
	if(closesocket(s) == SOCKET_ERROR) { \
		if(_close(s) != -1) { \
			s = 0; \
		} \
	} else { \
		s = 0; \
	}

#define O_RDONLY	_O_RDONLY
#define O_RDWR		_O_RDWR
#define O_APPEND	_O_APPEND
#define O_CREAT		_O_CREAT
#define O_BINARY	_O_BINARY

WORD ver = MAKEWORD(1,1);
WSADATA wsdata;
HANDLE pids[MAX_CHILD_NUM];
DWORD tids[MAX_CHILD_NUM];

#define signal(m,f) SetConsoleCtrlHandler((PHANDLER_ROUTINE)sig_int, TRUE)

#define SLAVE_INCRESE() slave++
#else

/**********************************************************************
 *                             LINUX
 *********************************************************************/
#include <utime.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define TRUE	1
#define FALSE	0

#define ERROR_ALREADY_EXISTS	1

#define O_BINARY 0

#define DIR_DELIM		'/'
#define WSAStartup(m,d) (0)
#define WSACleanup()	{}

#define SLAVE_INCRESE() { kill(getppid(),SIGUSR1); }
#endif

unsigned char *opt_list[] = { "-listen", "-slave", "-tran", "-log", "-x", "-q", NULL };

int child_num = 0;
int c_sock = 0, t_sock = 0;
struct sockaddr_in c_addr = {0,}, t_addr = {0,};
FILE *log_fp = NULL;
unsigned long g_flags = 0;

struct conn_info {
	int sock;
	unsigned char addr[16];
	unsigned short port;
};
struct conn_info c_info, t_info;
void *pids[MAX_CHILD_NUM];
int tids[MAX_CHILD_NUM];
int slave = 0;

void usage(const char *);
int socket_set(const char *, struct sockaddr_in *, const unsigned short);
int listen_proc(const char **);
int slave_proc(const char **);
int tran_proc(const char **);
int accept_client(int, struct sockaddr_in *);
void clean_up(int);
void sig_int(int);
void code_print(FILE *, const unsigned char *, int);
int Recv(int, void *, int, int);
int Send(int, const void *, int, int);
int Select(int, int);
int do_join(void);
int log_open(const char *);
void log_close(void);
void sig_child(int);
void fork_do_join(void);
int init_clients(void);
int pop_client(int);
int get_empty_client_idx(void);
void list_clients(void);

// FIXME: DoS?
void sig_usr1(int signo) { slave++; return; }

int main(int argc, char **argv)
{
	int i = 0, j = 0;
	unsigned char func = 0;
	int (*funcproc)(char **) = NULL;

	memset(&c_addr, 0x00, sizeof(c_addr));
	memset(&t_addr, 0x00, sizeof(t_addr));
	
	if(argc < 2) {
		goto failed;
	}

	while(argv[i] != NULL) {
		j = 0;
		while(opt_list[j] != NULL) {
			if(memcmp(argv[i], opt_list[j], strlen(argv[i])) == 0) {
				func = j + 1;
				break;
			}
			j++;
		}
		if(func == 1) {
			if(argc < 4) {
				goto failed;
			}
			funcproc = (void*)listen_proc;
		} else if(func == 2) {
			if(argc < 6) {
				goto failed;
			}
			funcproc = (void*)slave_proc;
		} else if(func == 3) {
			if(argc < 4) {
				goto failed;
			}
			funcproc = (void*)tran_proc;
		} else if(func == 4) {
			if(log_open(argv[i + 1]) < 0)
				goto failed;
			g_flags |= F_LOGW;
			i++;
		} else if(func == 5) {
			g_flags |= F_HEXW;
		} else if(func == 6) {
			fprintf(stdout, "[+] QUIET Mode\n");
			g_flags |= F_QUIET;
		}
		i++;
	}

	if((WSAStartup(ver, &wsdata)) != 0) {
		fprintf(stderr, "WSAStartup() function error\n");
		goto failed;
	}
	init_clients();

	signal(SIGINT, sig_int);
#ifndef _WIN32
	signal(SIGCHLD, sig_child);
	signal(SIGUSR1, sig_usr1);
#endif
	funcproc(argv);
	close(c_sock);
	close(t_sock);
	clean_up(0);
	return EXIT_SUCCESS;
failed:
	usage(argv[0]);
	clean_up(0);
}

void usage(const char *progname)
{
	fprintf(stdout,
		"[Usage of Packet Transmit:]\n"
		"  %s -<listen|tran|slave> <option> [-log logfile (-x)] -q\n\n"
		"[option:]\n"
		"  -listen <ConnectPort> <TransmitPort>\n"
		"  -tran   <ConnectPort> <TransmitHost> <TransmitPort>\n"
		"  -slave  <ConnectHost> <ConnectPort> <TransmitHost> <TransmitPort>\n\n", progname);
	return;
}

int socket_set(const char *server, struct sockaddr_in *addr_info, const unsigned short port)
{
	int s = 0, reuse = 1;
	struct hostent *he = NULL;

	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "[-] Socket create error.\n");
		goto failed;
	}

	if(server == NULL) { // server socket
		addr_info->sin_family = AF_INET;
		addr_info->sin_addr.s_addr = htonl(INADDR_ANY);
		addr_info->sin_port = htons(port);
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse,
				sizeof(reuse));
		if(bind(s, (struct sockaddr *)addr_info, sizeof(*addr_info)) < 0) {
			fprintf(stderr, "[-] Socket bind error.\n");
			goto failed;
		}
		listen(s, 5);
	} else {
		if((he = gethostbyname(server)) == NULL) {
			fprintf(stderr, "[-] gethostbyname() error.\n");
			goto failed;
		}
		memcpy(&addr_info->sin_addr, he->h_addr, he->h_length);
		addr_info->sin_family = AF_INET;
		addr_info->sin_port = htons(port);
		
		if(connect(s, (struct sockaddr *)addr_info, sizeof(*addr_info)) != 0) {
			fprintf(stderr, "[-] Connect error.\n");
			goto failed;
		}
	}
	
	return s;
failed:
	if(s > 0)
		close(s);
	return -1;
}

int listen_proc(const char **args)
{
	unsigned short c_port = 0, t_port = 0;

	memset(&c_info, 0x00, sizeof(c_info));
	memset(&t_info, 0x00, sizeof(t_info));

	c_port = atoi(args[2]);
	t_port = atoi(args[3]);

	// creating server socket
	fprintf(stdout, "[+] Listening port %d ......\n", c_port);
	if((c_sock = socket_set(NULL, (struct sockaddr_in *)&c_addr, c_port)) < 0) {
		return -1;
	} else {
		fprintf(stdout, "[+] Listen OK!\n");
	}

	fprintf(stdout, "[+] Listening port %d ......\n", t_port);
	if((t_sock = socket_set(NULL, (struct sockaddr_in *)&t_addr, t_port)) < 0) {
		return -1;
	} else {
		fprintf(stdout, "[+] Listen OK!\n");
	}
	
	while(1) {
		sig_child(0);

		fprintf(stdout, "[+] Waiting for Client on port:%d ......\n", c_port);
		if((c_info.sock = accept_client(c_sock, (struct sockaddr_in *)&c_addr)) < 0) {
			return -1;
		}
		fprintf(stdout, "[+] Accept a Client on port %d from %s ......\n", ntohs(c_addr.sin_port), inet_ntoa(c_addr.sin_addr));
		fprintf(stdout, "[+] Waiting another Client on port:%d....\n", t_port);
		if((t_info.sock = accept_client(t_sock, (struct sockaddr_in *)&t_addr)) < 0) {
			return -1;
		}
		fprintf(stdout, "[+] Accept a Client on port %d from %s ......\n", ntohs(t_addr.sin_port), inet_ntoa(t_addr.sin_addr));

		sprintf(c_info.addr, "%s", inet_ntoa(c_addr.sin_addr));
		sprintf(t_info.addr, "%s", inet_ntoa(t_addr.sin_addr));

		c_info.port = ntohs(c_addr.sin_port);
		t_info.port = ntohs(t_addr.sin_port);

		fprintf(stdout, 
			"[+] Accept connect OK!\n"
			"[+] Start Transmit (%s:%d <-> %s:%d) .....\n\n", 
			c_info.addr, c_info.port, t_info.addr, t_info.port);

		fork_do_join();

	} // end of while
	return 0;
}

int slave_proc(const char **args)
{
	while(1) {
		/*
		* XXX: Multi-Connection???„í•´?œëŠ” slave??ì§€?ì ???€ê¸?thread ?ì„±???„ìš”??
		*/
		sig_child(0);
			
		if(child_num >= MAX_CHILD_NUM || (child_num - slave) == 1) {
			sleep(1);
			continue;
		}

		memset(&c_info, 0x00, sizeof(c_info));
		memset(&t_info, 0x00, sizeof(t_info));
	
		fprintf(stdout, "[+] Make a Connection to %s:%d....\n", args[2], atoi(args[3]));
		if((c_info.sock = socket_set(args[2], &c_addr, atoi(args[3]))) < 0) {
			return -1;
		}

		sprintf(c_info.addr, "%s", inet_ntoa(c_addr.sin_addr));

		c_info.port = ntohs(c_addr.sin_port);

		fprintf(stdout, "[+] Connect OK!\n");
		fprintf(stdout, "[+] Make a Connection to %s:%d....\n", args[4], atoi(args[5]));
		if((t_info.sock = socket_set(args[4], &t_addr, atoi(args[5]))) < 0) {
			return -1;
		}

		sprintf(t_info.addr, "%s", inet_ntoa(t_addr.sin_addr));

		t_info.port = ntohs(t_addr.sin_port);

		fprintf(stdout, "[+] All Connect OK!\n"
				"[+] Start Transmit (%s:%d <-> %s:%d) ......\n\n",
				c_info.addr, c_info.port, t_info.addr, t_info.port);
		fork_do_join();
	} // end of while
	return 0;
}

int tran_proc(const char **args)
{
	unsigned short c_port = 0, t_port = 0;
	
	memset(&c_info, 0x00, sizeof(c_info));
	memset(&t_info, 0x00, sizeof(t_info));

	c_port = atoi(args[2]);
	t_port = atoi(args[4]);

	// creating server socket
	fprintf(stdout, "[+] Waiting for Client ......\n");
	if((c_sock = socket_set(NULL, (struct sockaddr_in *)&c_addr, c_port)) < 0) {
		return -1;
	}

	while(1) {
		sig_child(0);

		if((c_info.sock = accept_client(c_sock, (struct sockaddr_in *)&c_addr)) < 0) {
			return -1;
		}
		fprintf(stdout, "[+] Accept a Client from %s:%d ......\n", 
			inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

		sprintf(c_info.addr, "%s", inet_ntoa(c_addr.sin_addr));
	
		c_info.port = ntohs(c_addr.sin_port);

		fprintf(stdout, "[+] Make a Connection to %s:%d....\n", 
				args[3], atoi(args[4]));
	
		if((t_info.sock = socket_set(args[3], &t_addr, atoi(args[4]))) < 0) {
			return -1;
		}

		sprintf(t_info.addr, "%s", inet_ntoa(t_addr.sin_addr));
	
		t_info.port = ntohs(t_addr.sin_port);
		fprintf(stdout, "[+] Connect OK!\n"
				"[+] Start Transmit (%s:%d <-> %s:%d) ......\n\n",
				c_info.addr, c_info.port, t_info.addr, t_info.port);
		fork_do_join();
	}
	return 0;
}

int accept_client(int serv_s, struct sockaddr_in *addr_info)
{
	int s = 0, n = 0;
	unsigned short serv_port = 0;

	n = sizeof(*addr_info);

	serv_port = addr_info->sin_port;
	if((s = accept(serv_s, (struct sockaddr *)addr_info, &n)) == -1) {
		fprintf(stderr, "[-] Socket accept error.\n");
		return -1;
	}
	if(getpeername(s, (struct sockaddr *)addr_info, &n) < 0) {
		fprintf(stderr, "[-] getpeername() error.\n");
		return -1;
	}

	return s;
}

/*
 * XXX:
 * [+] OK! I Closed The Two Socket.
 */
void clean_up(int sig)
{
	if(c_sock != 0) {
		close(c_sock);
	}
	if(t_sock != 0) {
		close(t_sock);
	}
	if(sig) {
		fprintf(stdout, "[+] Let me exit ......\n");
		fprintf(stdout, "[+] All Right!\n\n");
	}
	if(g_flags & F_LOGW) {
		log_close();
	}
	WSACleanup();
	exit(EXIT_SUCCESS);
}

void sig_int(int signo)
{
	fprintf(stdout, "\n[-] Received Ctrl+C\n");
	clean_up(1);
}

void code_print(FILE *fp, const unsigned char *data, int sz)
{
	int i = 0, j = 0, c = 0, dec = 64;
	char buf[80];
	fprintf(fp, "%08X  ", i);
	for(i = 0; i < sz; i++) {
		if((i%16 == 0) && (i != 0)) {
			fprintf(fp, "\t");
			for(j = (i - 16); j != i; j++) {
				c = *(data + j);
				fprintf(fp, "%c", isprint(c) != 0 ? c : '.');
			}
			fprintf(fp, "\n%08X  ", i);
		}
		fprintf(fp, "%02X ", *(data + i));
	}
	if(i > j) {
		dec = dec - (((i - j) * 3) + 10);
		memset(buf, 0x00, sizeof(buf));
		memset(buf, 0x20, dec);
		fprintf(fp, "%s", buf);
		for(j; j < i; j++) {
			c = *(data + j);
			fprintf(fp, "%c", isprint(c) != 0 ? c : '.');
		}
	}
	fprintf(fp, "\n\n");
	return;
}

int Recv(int s, void *data, int sz, int flags)
{
	int n = 0;

	n = recv(s, data, sz, flags);
	
	if(n == 0) {
		fprintf(stderr, "[-] There is a error...Create a new connection.\n");
		goto lost_conn;
	} else if(n < 0) {
		fprintf(stderr, "[-] Read fd1 data error,maybe close?\n");
		goto lost_conn;
	}

	return n;
lost_conn:
	return -1;
}

int Send(int s, const void *data, int sz, int flags)
{
	int n = 0;
	
	if((n = send(s, data, sz, flags)) != sz) {
		close(s);
		fprintf(stderr, "[-] Read fd2 data error,maybe close?\n");
		return -1;
	}
	return n;
}

int do_join(void)
{
	int socks[2];
	char buf[1024];
	int len = 0, i = 0, exchange = 0, slave_inc = 0;
	char c_info_str[32], t_info_str[32];
	char *socket_info[2];

	socks[0] = c_info.sock;
	socks[1] = t_info.sock;

	sprintf(c_info_str, "%s:%d", c_info.addr, c_info.port);
	sprintf(t_info_str, "%s:%d", t_info.addr, t_info.port);

	socket_info[0] = (char*)&c_info_str;
	socket_info[1] = (char*)&t_info_str;

	while(1) {
		if(exchange > 10 && slave_inc == 0) {
			SLAVE_INCRESE(); slave_inc = 1;
		}
		if((i = Select(socks[0], socks[1])) < 0) goto failed;

		if((len = Recv(socks[i], buf, sizeof(buf) - 1, 0)) <= 0) {
			goto failed;
		}
		if(!(g_flags & F_QUIET)) fprintf(stdout, " Recv %5d bytes %21s\n", len, socket_info[i]);
	
		i = i ^ 0x01;
		
		if(Send(socks[i], buf, len, 0) != len) {
			goto failed;
		}
		if(!(g_flags & F_QUIET)) fprintf(stdout, " Send %5d bytes %21s\n", len, socket_info[i]);

		if((g_flags & F_LOGW) && (log_fp != NULL)) {
			fprintf(log_fp, "Recv %5d bytes from %21s\n", len, (socket_info[i ^ 0x01]));
			if(g_flags &F_HEXW) {
				code_print(log_fp, buf, len);
			} else {
				fprintf(log_fp, "%s\n", buf);
			}
		}
		exchange++;
	}
failed:
	if(socks[i] > 0)
		close(socks[i]);
	i = i ^ 0x01;
	if(socks[i] > 0)
		close(socks[i]);
	return -1;
}

int log_open(const char *logfile)
{
	unlink(logfile); // XXX: LOG FILE REMOVE
	umask(0077);
	if((log_fp = fopen(logfile, "w+")) == NULL) {
		fprintf(stderr, "[-] Can't open log file.\n");
		return -1;
	}
	fprintf(log_fp, "====== Start ======\n\n");
	return 0;
}

void log_close(void)
{
	if(log_fp != NULL) {
		fprintf(log_fp, "====== Exit ======\n\n");
		fclose(log_fp);
	}
	return;
}

int Select(int s1, int s2)
{
	struct timeval tv;
	fd_set read_fds;
	int socks[2], maxfd = 0, ret = 0, i = 0;

	socks[0] = s1;
	socks[1] = s2;

	while(1) {
		tv.tv_sec = 0;
		tv.tv_usec = 200000; // 0.2secs
	
		FD_ZERO(&read_fds);
		FD_SET(socks[0], &read_fds);
		FD_SET(socks[1], &read_fds);

		maxfd = socks[1] + 1;
		
		ret = select(maxfd, &read_fds, (fd_set*)0, (fd_set*)0, (struct timeval *)&tv);

		if(ret == -1) {
			if(errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "[-] Socket select error.\n");
				goto failed;
			}
		} else if(ret == 0) { // timeout
			continue;
		}

		for(i = 0; i < 2; i++) {
			if(FD_ISSET(socks[i], &read_fds)) {
				goto done;
			}
		}
	}
done:
	return i;
failed:
	return -1;
}

void sig_child(int signo)
{
	int pid = 0;
	int idx;

#ifdef _WIN32
	if(child_num != 0) {
		idx = WaitForMultipleObjects(child_num, pids, FALSE, 0);
		if(idx != WAIT_TIMEOUT) {
			fprintf(stdout, "[!] Killed Thread: %d\n", tids[idx]);
			pop_client(idx);
			child_num--; slave--;
		}
	}
#else	
	if(signo == 0) return;
	while((pid = waitpid(-1, NULL, WNOHANG|WUNTRACED)) < 0);
	fprintf(stdout, "[!] Killed a child process: %d\n", pid);
	pop_client(pid);
	child_num--; slave--;
#endif
	return;
}

void fork_do_join(void)
{
	int pid = 0;
	int idx = 0;

	if((idx = get_empty_client_idx()) == -1) {
		fprintf(stderr, "[!] Maximum child processes!\n");
		return;
	}

#ifdef _WIN32
	if((pids[idx] = CreateThread(NULL, 0, (DWORD WINAPI)do_join, 0, 0, &tids[idx])) == NULL) {
		fprintf(stderr, "[-] CreateThread failed.\n");
		return;
	}
	fprintf(stdout, "[+] Create a thread: %d\n", tids[idx]);
	child_num++;
	list_clients();
	sleep(1);
#else
	pid = fork();
	switch(pid) {
	case -1:
		fprintf(stderr, "[-] fork() failed.\n");
		exit(0);
	case 0: // child
		do_join();
		exit(0);
	default: // parent
		fprintf(stdout, "[+] Created a child process: %d\n", pid);
		if(c_info.sock > 0) { close(c_info.sock); c_info.sock = 0; }
		if(t_info.sock > 0) { close(t_info.sock); t_info.sock = 0; }
		pids[idx] = NULL; tids[idx] = pid;
		child_num++;
		list_clients();
		break;
	}
#endif
	return;
}

int init_clients(void)
{
	int i = 0;
	
	for(i = 0; i < MAX_CHILD_NUM; i++) {
		pids[i] = NULL;
		tids[i] = 0;
	}
	return i;
}

int get_empty_client_idx(void)
{
	int i = 0;
	for(i = 0; i < MAX_CHILD_NUM; i++) {
		if(pids[i] == NULL && tids[i] == 0) return i;
	}
	return -1; // XXX: there is no empty!
}

// XXX: Windows - IDX, Linux - pid
// Windows - if(idx == ${USER_VALUE}) { ... }
// Linux - if(tids[idx] == ${USER_VALUE}) { ... } 
int pop_client(int idx)
{
	int i = 0, modified = 0;
#ifdef _WIN32
	for(i = 0; i < MAX_CHILD_NUM; i++) {
		if(i == idx) {
			CloseHandle(pids[i]);
			pids[i] = NULL;
			tids[i] = 0;
			break;
		}
	}
	modified = i;
	i++;
	for(i; i < MAX_CHILD_NUM; i++) {
		pids[i - 1] = pids[i];
		tids[i - 1] = tids[i];
	} 
#else
	for(i = 0; i < MAX_CHILD_NUM; i++) {
		if(tids[i] == idx) {
			pids[i] = NULL;
			tids[i] = 0;
			break;
		}
	}
	modified = i;
	i++;
	for(i; i < MAX_CHILD_NUM; i++) {
		pids[i - 1] = pids[i];
		tids[i - 1] = tids[i];
	} 
#endif
	list_clients();
	return modified;
}

void list_clients(void)
{
	int i = 0;
#ifdef _DEBUG
	fprintf(stdout, "-- LIST CLIENTS(%d) --\n", slave);
	for(i = 0; i < MAX_CHILD_NUM; i++) {
		fprintf(stdout, "[%d] PID: %p, TID: %d\n", i, pids[i], tids[i]);
	}
	fprintf(stdout, "-- LIST CLIENTS(%d) --\n", slave);
#endif
	return;
}
