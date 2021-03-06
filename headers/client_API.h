/* DESCRIZIONE API CLIENT */

#ifndef client_API_h
#define client_API_h

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "err_cleanup.h"
#include "definitions.h"
#include "util.h"
#include "client_server_comm.h"
#include "part_rw_sol.h"

#define RECONNECT_TIMER 200     // time to wait between one connection attempt and another
#define LOCK_TIMER 500      // time to wait between one lock attempt and another
#define OP_ERR -2   // denotes non-fatal errors (client-server communication errors)

extern short fd_sock;       // holds the file descriptor representing the connection to the server
extern char* serv_sk;        // holds the address to which the clients connect in the openConnection call
extern bool is_connected;       // set to 1 when the connection with the server is established, 0 otherwise

char* save_dir;    // used to specify the folder in which to save files retrieved from the file-server
char* miss_dir;    // used to specify the folder in which to save files discarded by the file-server


/* ########################## Main functions ################################## */

int openConnection(const char* sockname, int msec, const struct timespec abstime);

int closeConnection(const char* sockname);

int openFile(const char* pathname, int flags);

int readFile(const char* pathname, void** buf, size_t* size);

int readNFiles(int N, const char* dirname);

int writeFile(const char* pathname, const char* dirname);

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

int lockFile(const char* pathname);

int unlockFile(const char* pathname);

int closeFile(const char* pathname);

int removeFile(const char* pathname);


#endif // client_API_h