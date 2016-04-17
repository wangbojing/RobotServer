/*******************************************************************************
 *
 * Copyright (C) u-blox ag
 *
 * u-blox ag
 * Zuercherstrasse 68
 * CH-8800 Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR U-BLOX MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 *******************************************************************************
 *
 * Project: u-blox 5
 *
 ******************************************************************************/
/*!
  \mainpage Assist Now Online Client Sample Code

  This project implements an example client functionality for 
  Assist Now Online. This software is shortnamed "anolclient".


  At startup, within main(), it opens the serial port to a GPS receiver

  It then goes to an endless loop, where it will
  - restart the GPS receiver
  - request and receive Assist Now Online AGPS data (see getAssistNowOnlineData())
  - forward that data to the GPS receiver

  \image html anolclient.png

  All status and debug output to the console will 
  go to standard error, using fprintf(stderr,....)

  Please note that getAssistNowOnlineData() allows both 
  UDP and TCP data exchange with the server. Which one to 
  use depends on the application and network link:

  - TCP is beneficial over unreliable links, as it will ensure that all data is being exchanged. However, TCP connections have a larger overhead, as connection setup and shutdown need additional data exchange
  - UDP has the benefit of single requests and reply messages being sent. No connection is set up, and therefore no overhead is needed. However, there is no guarantee that data is delivered successfully


  The code tries to rely on POSIX and standard C 
  language libraries only, in order to reach 
  portability. However, the serial port functions 
  are not portable and should be replaced with your 
  operating system's functions.

  This code has been successfully tested on linux devices.
  (little endian. big endian systems might not work right out 
  of the box)

  It can be compiled as follows:
	
  \verbatim
  gcc -g -Wall -o anolclient anolclient.c
  \endverbatim

  In order to use it, you need to have a Assist Now Online Account.
  (username and password), or use your own proxy server.
  In case you do not have such an account, a email to 
  agps-account@u-blox.com will generate one (you will get 
  a reply email with a password)



  The command can be executed as follows:

  \verbatim
  ./anolclient -g /dev/ttyACM0 -s agps.u-blox.com:46434 -u <you@some.domain> -p <yourpassword> -la 47.28 -lo 8.56
  \endverbatim

   

  /dev/ttyACM0 would be a u-blox 5 receiver connected via USB,

   -  -u and -p are your username/password
   -  -la and -lo set latitude and longitude of your approximate location
   -  -h would give some more help options


  \file
  Assist Now Only Client Sample Code
	

*/
/*******************************************************************************
 * $Id: anolclient.c 34721 2009-08-06 06:54:43Z daniel.ammann $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/PRODUCTS/AssistNowOnlineSampleClient/anolclient.c $
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

#include "ubx.h"
#include "ApolloUtil.h"
#include "ApolloProtocol.h"
#include "ApolloSyner.h"


#define BUFFER_SIZE 8192	//!< Size of the temporary buffers
#define MAXFIELDLEN 128     //!< Maximum length for username and password

#define ANOL_UDP 0 //!< Used in getAssistNowOnlineData() to indicate UDP traffic
#define ANOL_TCP 1 //!< Used in getAssistNowOnlineData() to indicate TCP traffic

#define AGPS_SAVE_FILE		1
#define AGPS_SAVE_UART		2

#define UART_DEVICE_NAME		"/dev/ttyUSB0"
#define AGPS_FILE_NAME			"agps.dat"

#define AGPS_SAVE_TYPE		AGPS_SAVE_FILE

typedef struct REQ_s {
	CH username[MAXFIELDLEN]; //!< Username to be sent to server
	CH password[MAXFIELDLEN]; //!< Password to be sent to server
	R8 lat; //!< Latitude of approximate location (in decimal degrees)
	R8 lon; //!< Longitude of approximate location (in decimal degrees)
	R8 alt; //!< Altitude of approximate location (in meters, can be zero)
	R8 accuracy; //!< Accuracy of lat/lon, in meters
} REQ_t;

//! Initialize Serial Port
/*!
  This initializes a serial port connected to a PC.
  Depending on your operating system, this might need 
  modifications or a rewrite

  This code has been tested using Linux, both with real 
  serial ports (e.g. /dev/ttyS0) as well as Serial-over-USB
  (using ACM devices, such as /dev/ttyACM0)

  Further, throughout the code, the serial port is 
  initialized, read and write using the functions 
  open(), read() and write(). This functions shall be replaced 
  with corresponding functions of your operating system
*/
I setSerialParams(I gpsDes, I baudrate);

//------------------------------------------------------------------------------
//! getAssistNowOnlineData
/*! 
  This is the main function that retrieves Assist Now Online data

  It does 
  - a name lookup
  - connects to the server
  - sends the request
  - interprets and returns the result

  \param connectTo Hostname and Port to connect to (in host:port) syntax
  \param proto Protocol to use (either ANOL_UDP or ANOL_TCP)
  \param cmd Assist Now Online command (must be "aid", "eph", "full" or "alm")
  \param userInfo pointer to a REQ_t structure, containing user credentials and approximate location
  \param buffer pre-allocated buffer where result data should be copied to
  \param bufferLength Maximum size of buffer
  \return number of bytes copied to the buffer. <=0 on error
*/


I getAssistNowOnlineData(CH * connectTo,I proto,CH * cmd,REQ_t * userInfo, CH * buffer, I bufferLength);




//------------------------------------------------------------------------------
I setSerialParams(I gpsDes, I baudrate)
{
	struct termios  termios;
	speed_t Bbaudrate = 9600;


	switch(baudrate)
	{
	case 115200: Bbaudrate = B115200;break;
	case 57600:  Bbaudrate = B57600 ;break;
	case 38400:  Bbaudrate = B38400 ;break;
	case 19200:  Bbaudrate = B19200 ;break;
	case 4800:   Bbaudrate = B4800  ;break;
	default:   
		Bbaudrate = B9600;
	}

	// Set serial port parameters
	if (tcgetattr(gpsDes, &termios) < 0)
	{
		fprintf(stderr,"ERROR: tcgetattr fails: %s\n",strerror(errno));
		return (0);
	}
	termios.c_iflag = 0;
	termios.c_oflag = 0;        /* (ONLRET) */
	termios.c_cflag = CS8 | CLOCAL | CREAD;
	termios.c_lflag = 0;
	{
		int             cnt;
		
		for (cnt = 0; cnt < NCCS; cnt++)
			termios.c_cc[cnt] = -1;
	}
	termios.c_cc[VMIN] = 100;
	termios.c_cc[VTIME] = 10;	
	if ((cfsetispeed(&termios, B115200) != 0) || (cfsetospeed(&termios, B115200) != 0))
	{
		fprintf(stderr,"ERROR: cfset[io]speed fails: %s\n",strerror(errno));
		return 0;

	}

	if (tcsetattr(gpsDes, 0, &termios) < 0) {
		fprintf(stderr,"ERROR: tcsetattr: %s\n",strerror(errno));
		return 0;
	}
	if (tcflush(gpsDes, TCIOFLUSH) < 0) {
		fprintf(stderr,"ERROR: tcflush: %s\n",strerror(errno));
		return 0;
	}
	if (fcntl(gpsDes, F_SETFL, 0) == -1) {
		fprintf(stderr,"ERROR: fcntl: %s\n",strerror(errno));
		return 0;
	}
	return 1;
}



I getAssistNowOnlineData(CH * connectTo,I proto,CH * cmd,REQ_t * userInfo, CH * buffer, I bufferLength)
{
	struct sockaddr_in si_me;
	int agpsSocket;
	CH peerName[256];
	unsigned int peerPort; // UDP or TCP port to use
	CH request[256]; // Variable to hold the request string
	CH reply[BUFFER_SIZE];
	// parse host/port string
	CH * delim = strchr(connectTo,':');
	I4 totalLength = 0;
	I4 expectedLength = 1e7;
	I4 payloadLength = 0;
	CH * pPayload = NULL;
	
	// split the connectTo string into host:port parts
	if (delim)
	{
		*delim = (char)0;
		strcpy(peerName,connectTo);
		peerPort = atoi(delim+1);
		*delim = ':';
	}
	else
	{
		strcpy(peerName,connectTo);
		peerPort = 46434;
	}

	// *****************************************************
	// First, lookup IP address.
	//
	// Please note that you NEED to do a IP lookup. This, because
	// the IP number of the server may change 
	// *****************************************************

	memset((char *) &si_me, 0, sizeof(struct sockaddr_in));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(peerPort);
	unsigned long addr = inet_addr(peerName);
	if (addr != INADDR_NONE)
	{
		// Numeric IP Address
		memcpy((char *)&si_me.sin_addr.s_addr, &addr, sizeof(addr));
	}
	else
	{
		struct hostent* hostInfo = gethostbyname(peerName);
		if (hostInfo != NULL)
		{
			memcpy((char *)&si_me.sin_addr.s_addr, hostInfo->h_addr, hostInfo->h_length);
		}
		else
		{
			fprintf(stderr,"Unable to look up %s - exiting\n",peerName);
			return 0;
		}
	}


	// *****************************************************
	// Then, open the socket and connect.
	//
	// Please note that this is the *only* place where TCP and UDP
	// are handled differently
	// *****************************************************

	if (proto == ANOL_UDP)
	{
		if ((agpsSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		{
			fprintf(stderr,"unable to create UDP socket: %s\n",strerror(errno));
			return 0;
		}
	} 
	else if (proto == ANOL_TCP)
	{
		if ((agpsSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==-1)
		{
			fprintf(stderr,"unable to create TCP socket: %s\n",strerror(errno));
			return 0;
		}		
	} else {
		fprintf(stderr,"invalid protocol type %i\n",proto);
		return 0;
	}


	if (connect(agpsSocket,(struct sockaddr *)&si_me,sizeof(struct sockaddr_in))<0)
	{
		fprintf(stderr,"unable to connect: %s\n",strerror(errno));
		return 0;
	}


	// Set up a timeout for the socket
	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	if (setsockopt(agpsSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof(tv)))
	{
		fprintf(stderr,"setsockopt: %m");
		return 0;
	}


	// *****************************************************
	// Then, create the request string and send it
	// *****************************************************

	sprintf(request,"user=%s;pwd=%s;cmd=%s;lat=%.4f;lon=%.4f;pacc=%.0f",
		   userInfo->username,
		   userInfo->password,
		   cmd,
		   userInfo->lat,
		   userInfo->lon,
		   userInfo->accuracy);

	fprintf(stderr,"sending %s request to %s:%i (%s)\n",
		   proto == ANOL_TCP?"TCP":"UDP",
		   peerName,peerPort,
		   inet_ntoa(si_me.sin_addr)
		   );
	totalLength = write(agpsSocket,request,strlen(request));
	if (totalLength<0)
	{
		fprintf(stderr,"unable to send: %s\n",strerror(errno));
		return 0;
	}

	// *****************************************************
	// Then, read the server's replay 
	// (or run into the timeout)
	// *****************************************************

	totalLength = 0;

	// a single read call is not good enough, as the data may 
	// be received fragmented or slowly (in the TCP case) 

	// When we begin reading data, we do not know yet how much
	// we are going to get, so we initially set the expectedLength
	// to a large value.
	// Once we know more (because we have received the header),
	// we can set the expectedLength to the then known value
	while( (totalLength<expectedLength) && (totalLength<BUFFER_SIZE))
	{
		// Read Data from the socket
		I4   readLength = read(agpsSocket, reply+totalLength, BUFFER_SIZE-totalLength);
		CH * headerEnd;

		// If we have not received anything....
		if (readLength <= 0)
		{
			if (!readLength)
			{
				fprintf(stderr,"timeout while reading from socket!\n");

				return 0;
			} else {
				fprintf(stderr,"error reading from socket: %i %m!\n",readLength);
			}
			return -1;
		} 
		totalLength += readLength;
		
		// While reading from the socket, we want to parse it, in order to see whether
		// the full header has already been received.
		// If it has, extract the Payload size from the header, and 
		// set the expectedLength to that of the header+payload size
		// Also, set some pointer and counters for later use
		headerEnd=strstr(reply,"\r\n\r\n");
		if (headerEnd)
		{
			CH * pContentLength = strstr(reply,"Content-Length");
			U4 contentLength=0;
			if (sscanf(pContentLength,"Content-Length: %u",&contentLength) && (contentLength>0))
			{
				expectedLength = headerEnd - reply + contentLength + 4 /* for \r\n\r\n */;
				payloadLength  = contentLength; // store for later
				*headerEnd = (CH)0; // split into two strings
				pPayload = headerEnd + 4; // store for later
				fprintf(stderr,"setting exptected length to %i bytes\n",expectedLength);
			}
		}
	}

	// Close the socket - nothing more to do on the network side
	close(agpsSocket);


	// *****************************************************
	// Parse the reply, check for errors 
	// and return the payload 
	// *****************************************************
	if ((expectedLength == totalLength) && pPayload && payloadLength)
	{
		fprintf(stderr,"received %i bytes okay\n",totalLength);
		
		// If the header is Content-Type application/ubx, 
		// --> we can forward to the GPS receiver
		if (strstr(reply,"Content-Type: application/ubx"))
		{
			if (payloadLength>bufferLength)
			{
				fprintf(stderr,"Buffer is too small (%i), received %i bytes. \n",bufferLength,payloadLength);
				return -2;
			} else {
				fprintf(stderr,"ubx data with %i bytes payload\n",payloadLength);
				memcpy(buffer,pPayload,payloadLength);
				return payloadLength;
			}
			
		} else {
			// If the header is Content type text/plain, 
			// something went wrong
			if (strstr(reply,"Content-Type: text/plain"))
			{
				fprintf(stderr,"ERROR: %s\n",pPayload);
				return -3;
			} else {
				// Something went wrong - server does not adhere to our protocol ?!?
				return -4;
			}
		}
	} else {
		// Something went wrong - server does not adhere to our protocol? Not an AGPS Server ?!?
		return -5;
	}
}

extern int set_key_toredis(const char *key, const char *value);

void* ubloxDownloadApgs(void* arg)
{	
	CH fileName[MAXFIELDLEN] = {0};
	CH cmdShell[MAXFIELDLEN] = {0};
	CH   connectTo[MAXFIELDLEN] = "agps.u-blox.com:46434";
	I    i; 
	REQ_t req = { "wangbojing1989@foxmail.com", "Iusrpo", 23.0744,113.14028,0,1500e3}; //<
	I    count=1, size;  // Just to count the number of restarts - and to cycle between different aiding modes and protocols
	LocationInfo *locationInfo = (LocationInfo*)arg;
	I	 gpsDes = -1;  // File description to GPS device
	CH u8Cmd[12] = {0};
	MulticastSynerPacket *pSynPacket = NULL;

	req.lat = atof(locationInfo->u8Latitude+1);
	req.lon = atof(locationInfo->u8Longitude+1);
	printf(" Deiv:%s, lat:%f, lng:%f\n", locationInfo->u8DeviceId, req.lat, req.lon);
#if (DATA_SAVE_TYPE == DISTRBUTE_SAVE)
	sprintf(fileName, "/usr/RServer/agps_data/%s_%s", locationInfo->u8DeviceId, AGPS_FILE_NAME);
#elif (DATA_SAVE_TYPE == DISK_FILE_SAVE)
	sprintf(fileName, "agps_data/%s_%s", locationInfo->u8DeviceId, AGPS_FILE_NAME);
#else
	sprintf(fileName, "agps_data/%s_%s", locationInfo->u8DeviceId, AGPS_FILE_NAME);
#endif
	gpsDes = open(fileName, O_RDWR | O_CREAT);
	if (gpsDes < 0) {
		perror(AGPS_FILE_NAME);  
		return NULL;
	}
	
	CH buffer[BUFFER_SIZE];  // temporary buffer
	I  len;
	I  timeout = 20; 
	

	tcflush(gpsDes,TCIOFLUSH); // Flush the Send/Receive queues to the GPS receiver

	fprintf(stderr,"AssistNow Online Request %i\n",++count);
	
	if ((len=getAssistNowOnlineData(connectTo,
							  ANOL_TCP, //count&0x01 ? ANOL_TCP:ANOL_UDP, // Switch between TCP and UDP protocol in this example code
							  "aid", // Switch between "full" and "aid" modes. 
							  &req,  // User credentials and approximate location 
							  buffer+8,BUFFER_SIZE))   >0  )
	{
		// We have successfully receiverd data from the server, which is 
		// now within 'buffer', for up to 'len' bytest
		fprintf(stderr,"-> forwarding %i bytes to GPS receiver\n",len);
		// We are sending this data right away to the GPS receiver
		//write(gpsDes,buffer,len);
		//write file
		//#if (AGPS_SAVE_TYPE == AGPS_SAVE_FILE) //write file
		//#elif (AGPS_SAVE_TYPE == AGPS_SAVE_UART) //write serial port	
		writeTimeHeader(buffer);
		size = write(gpsDes,buffer,len+8);
		if (size < 0) {
			perror("write:");  
		}
		tcflush(gpsDes,TCIOFLUSH);
		//#endif
	} else {
		// Some error occurred. See getAssistNowOnlineData() for possible error codes
		fprintf(stderr,"-> failed. error code %i\n",len);
	}
	close(gpsDes);

	sprintf(cmdShell, "chmod 777 %s", fileName);
	system(cmdShell);

	printf(" u8DeviceId:%s\n", locationInfo->u8DeviceId);
	sprintf(u8Cmd, "%d", CMD_PHONE_DEVICE_AGPS_DOWNLOAD);
	if (-1 == set_key_toredis(locationInfo->u8DeviceId, u8Cmd)) {
		return NULL;
	}
	
	//multicast
	pSynPacket = (MulticastSynerPacket*)malloc(sizeof(MulticastSynerPacket));
	memset(pSynPacket, 0, sizeof(MulticastSynerPacket));
	strcpy(pSynPacket->u8DeviceId, locationInfo->u8DeviceId);
	pSynPacket->u8flag = MULTICAST_TYPE_AGPS;
	StartApolloSynerAction((void*)pSynPacket);
	
	free(locationInfo);
	return NULL;
}

