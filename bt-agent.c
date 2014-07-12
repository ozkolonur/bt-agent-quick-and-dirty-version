/*
 CHANGELOG
 ---------
 12/12/2010 - Initial Version.


*/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/signal.h>
#include <curl/curl.h>
#include <time.h>
#include "hexdump.h"
#define TRACE printf("::%d::%s::\n", __LINE__, __func__)

#define BUFFER_SIZE 16 * 1024

#define ST_SCANNING		0
#define ST_CONNECTING	1
#define ST_CONNECTED	2
#define RES_INVALID		0
#define RES_VALID		1
#define RES_COMMITTED	2
#define TRUE 			1
#define FALSE 			0

struct termios options;
int init_serial(char *device);
int data_arrived = 0;
int state = 0;
int mres = RES_INVALID;
int sys = 0;
int dia = 0;
int hb = 0;

/* Server response buffers */
struct MemoryStruct {
  char *memory;
  size_t size;
};


/*
*  Returns fd
*/
int spp_open(char *device)
{
    int fd_new=-1;
    fd_new = open(device, O_RDONLY | O_NOCTTY | O_NONBLOCK);

    if(fd_new < 0)
		return fd_new;

	fcntl(fd_new, F_SETFL, FASYNC);
    tcgetattr(fd_new, &options);
    options.c_cflag     |= (CLOCAL | CREAD);
    options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag     &= ~OPOST;
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 150;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    tcsetattr(fd_new, TCSANOW, &options);

    return fd_new;
}


int spp_close(int fd)
{
	close(fd);
	fd = -1;
}

void signal_handler_IO(int status)
{        
	printf(".");
	fflush(stdout);
    data_arrived = TRUE;
}
 
 
static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }
 
  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}


int extract_measurement(unsigned char buffer[BUFFER_SIZE])
{
	//printf("%s::%d\n", __func__, __LINE__);
	//hexdump(buffer, sizeof(buffer));
	int i = 0;
	for(i=0; i<BUFFER_SIZE; i++)
		if(buffer[i] == 1)
			if(buffer[i+1] == 2)
				if(buffer[i+2] == 255)
					if(buffer[i+6] == 1)
						if(buffer[i+7] == 3)
						{
							printf("\nbt-agent: Measurement Received\n");
							sys = (int)buffer[i+3];
							dia = (int)buffer[i+4];
							hb = (int)buffer[i+5];
							mres = RES_VALID;
							return mres;
						}
}

void send_measurement(char *email, char *password, char *bpm_mac)
{
	CURL *curl;
	CURLcode res;
	struct MemoryStruct response;
	char url[512];
  	

http://well.io/mining_platform/http_get/?password=123123&email=admin@admin.com&hb=79&dia=80&sys=117&bpm_mac=00:1C:97:33:00:8C&mac_addr=00:1C:97:33:00:8C

    		//printf("bt-agent: sys = %d, dia = %d , hb = %d mres=%d\n", sys, dia, hb, mres);

	if (mres==RES_VALID)
	{
		printf("bt-agent: sys = %d, dia = %d , hb = %d\n", sys, dia, hb);

        sprintf(url, 
			"http://well.io/mining_platform/http_get/?email=admin@admin.com&password=123123&sys=%d&dia=%d&hb=%d&bpm_mac=%s&mac_addr=%s\n", 
			sys, dia, hb, bpm_mac, bpm_mac);

		printf("%s\n",url);
      
		response.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  		response.size = 0;    /* no data at this point */ 

  		curl_global_init(CURL_GLOBAL_ALL);

    	curl = curl_easy_init();
		if(curl) 
		{    
			curl_easy_setopt(curl, CURLOPT_URL, url); 	   

		  	/* send all data to this function  */ 
  			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
  			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
 
  			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

			res = curl_easy_perform(curl);    /* always cleanup */    

			curl_easy_cleanup(curl);
			
			printf("%lu bytes response retrieved\n", (long)response.size);

			printf("Server Response[%p]=%s\n", response.memory, response.memory);

            if (strstr(response.memory, "OK") != NULL )
			{
				printf("Measurement Committed to server [took x seconds]\n");
				mres = RES_COMMITTED;
			} else if (strstr(response.memory, "FAIL") != NULL)
			{
				printf("Server responded negatively - check login info\n");
				mres = RES_COMMITTED;
			}

				
 			if(response.memory)
    			free(response.memory);
 
  			/* we're done with libcurl, so clean it up */ 
  			curl_global_cleanup();
  
		}
		printf("\n");  
		return;
	}

}

int main(int argc, char *argv[])
{
    int retval;
	unsigned char buffer[BUFFER_SIZE];
	char *bufptr = buffer;
	int nbytes;
	struct sigaction saio;           /* definition of signal action */
    int fd;
	time_t con_time, cur_time;

    if(argc<2)
    {    
        printf("\nUsage: bt-agent bt_mac_id email password\n\n");
		return 0;
    }

    printf("bt-agent: Target bluetooth mac adress	:%s\n",argv[1]);
    printf("bt-agent: Target service user	\t:%s\n",argv[2]);
    
    /* install the signal handler before making the device asynchronous */        
	saio.sa_handler = signal_handler_IO;        
	//saio.sa_mask = 0;        
	saio.sa_flags = 0;        
	saio.sa_restorer = NULL;        
	sigaction(SIGIO,&saio,NULL);

    /* allow the process to receive SIGIO */        
	fcntl(fd, F_SETOWN, getpid());

	/* HCI Initialization should be done here*/
	

    while(1)
    {
        state = ST_CONNECTING;
   	 	if( (fd = spp_open("/dev/rfcomm0")) < 0)
    	{
			if (errno == 0x70) /* Host is down */
			{
				printf("bt-agent: BT Host is down\n");
				continue;
			}
			else
			{
				printf("bt-agent: Unknown Error, errno = %x\n", errno);
			}
    	} 
		else
		{
			state = ST_CONNECTED;
			printf("bt-agent: Connected to BT Device [%s]\n", argv[1]);
        }
        data_arrived = FALSE;
		con_time = time(NULL);
		while(state == ST_CONNECTED)
		{
			//printf("%s::%d\n", __func__, __LINE__);
			
			if (data_arrived == TRUE)
			{
			//printf("%s::%d\n", __func__, __LINE__);
				nbytes = read(fd,bufptr,BUFFER_SIZE);
				bufptr += nbytes;
				data_arrived = FALSE;
				con_time = time(NULL);
				if (extract_measurement(buffer) == RES_VALID)
				{
					send_measurement(argv[2], argv[3], argv[1]);
					
					while(mres != RES_COMMITTED)
					{
						send_measurement(argv[2], argv[3], argv[1]);
						sleep(1);
					}	

					sys = 0;
					dia = 0;
					hb = 0;
					mres = RES_INVALID;

					/* do something here */
					spp_close(fd);
    				memset(buffer, 0, BUFFER_SIZE);
					bufptr = buffer;
					sleep(4);
					break;
				}
			}
			//printf("%s::%d\n", __func__, __LINE__);
		    cur_time = time(NULL);
			if(cur_time - con_time > 30)
			{
				spp_close(fd);
				memset(buffer, 0, BUFFER_SIZE);
				bufptr = buffer;
				state = ST_CONNECTING;
				printf("bt-agent: Disconnected from device [%s]\n", argv[1]);
				break;
			}
				
			/* if no event in specified time, check connection */
			usleep(100000);
		}
		sleep(1);
    }

    close(fd);
    return 0;
}

