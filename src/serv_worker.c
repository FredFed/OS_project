#include "serv_worker.h"

static int worker_file_open(int);
static int worker_file_read(int);
static int worker_file_readn(int);
static int worker_file_write(int);
static int worker_file_write_app(int);
static int worker_file_lock(int);
static int worker_file_unlock(int);
static int worker_file_remove(int);
static int worker_file_close(int);

// thread main
void* worker_func(void* arg) {

    int* id=(int*)arg;
    long unsigned total_requests_num=0;
    int temperr;    // used for error handling
    int* int_buf=NULL;   // serves as a buffer for int values
    int client_fd;     // will hold the fd of the user that sent the request
    int op;     // will hold the operation code representing the current request

    while(TRUE) {

        if((temperr=pthread_mutex_lock(&(requests_queue->queue_mtx)))==ERR)
        {LOG_ERR(temperr, "worker: locking req queue"); exit(EXIT_FAILURE);}

        while(!(int_buf=(int*)conc_fifo_pop(requests_queue)) && !(hard_close)) {  // checking for new requests
            if(errno) {LOG_ERR(errno, "worker: reading from req queue"); exit(EXIT_FAILURE);}

            if((temperr=pthread_cond_wait(&(requests_queue->queue_cv), &(requests_queue->queue_mtx)))==ERR)
            {LOG_ERR(temperr, "worker: waiting on req queue"); exit(EXIT_FAILURE);}
        }

        if((temperr=pthread_mutex_unlock(&(requests_queue->queue_mtx)))==ERR)
        {LOG_ERR(temperr, "worker: unlocking req queue"); exit(EXIT_FAILURE);}

        if(hard_close) break;   // server must terminate immediately

        client_fd=*int_buf;    // preparing to serve the user's request
        *int_buf=0;
        if(!int_buf) {LOG_ERR(errno, "worker: preparing op code read"); exit(EXIT_FAILURE);}

        // getting the request into int_buf
        if((temperr=readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) {
            LOG_ERR(EREMOTEIO, "worker: reading client operation");
            *int_buf=0; // disconnecting the client as a result of the error
        }

        op=*int_buf;    // reading the operation code

        if(op==EOF || op==0) {   // the client closed the connection
            *int_buf=client_fd;
            (*int_buf)*=-1; // tells the manager to close the connection with this client
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe");}
        }
        else if(op==OPEN_F) {
            if((temperr=worker_file_open(client_fd))==ERR)
            {LOG_ERR(errno, "worker: open file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (open)");}
        }
        else if(op==READ_F) {
            if((temperr=worker_file_read(client_fd))==ERR)
            {LOG_ERR(errno, "worker: read file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (read)");}
        }
        else if(op==READ_N_F) {
            if((temperr=worker_file_readn(client_fd))==ERR)
            {LOG_ERR(errno, "worker: readn file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (readn)");}
        }
        else if(op==WRITE_F) {
            if((temperr=worker_file_write(client_fd))==ERR)
            {LOG_ERR(errno, "worker: write file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (write)");}
        }
        else if(op==WRITE_F_APP) {
            if((temperr=worker_file_write_app(client_fd))==ERR)
            {LOG_ERR(errno, "worker: write_app file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (write_app)");}
        }
        else if(op==LOCK_F) {
            if((temperr=worker_file_lock(client_fd))==ERR)
            {LOG_ERR(errno, "worker: lock file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (lock)");}
        }
        else if(op==UNLOCK_F) {
            if((temperr=worker_file_unlock(client_fd))==ERR)
            {LOG_ERR(errno, "worker: unlock file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (unlock)");}
        }
        else if(op==RM_F) {
            if((temperr=worker_file_remove(client_fd))==ERR)
            {LOG_ERR(errno, "worker: remove file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (remove)");}
        }
        else if(op==CLOSE_F) {
            if((temperr=worker_file_close(client_fd))==ERR)
            {LOG_ERR(errno, "worker: close file error"); exit(EXIT_FAILURE);}
            total_requests_num++;

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (close)");}
        }
        else {
            *int_buf=EBADR;
            if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) 
            {LOG_ERR(EREMOTEIO, "worker: replying to client's bad request");}

            *int_buf=client_fd;    // tells the manager the operation is complete
            if((temperr=writen(fd_pipe_write, (void*)int_buf, PIPE_MSG_LEN))==ERR)
            {LOG_ERR(EPIPE, "worker: writing to the pipe (close)");}
        }
        if(int_buf) {free(int_buf); int_buf=NULL;}  // resetting int_buf to NULL value
    }

    if((temperr=pthread_mutex_lock(&(log_queue->queue_mtx)))==ERR) {
        LOG_ERR(temperr, "logging: locking log queue"); exit(EXIT_FAILURE);
    }
    
    int logfile_fd;
    if((logfile_fd=open(log_file_path, O_WRONLY | O_APPEND | O_CREAT, 0777))==ERR) {
        LOG_ERR(errno, "logger: opening log file");
        exit(EXIT_FAILURE);
    }

    dprintf(logfile_fd, "##########\nWORKER %d served %lu requests\n##########\n\n", *id, total_requests_num);

    if((close(logfile_fd))==ERR) {
        LOG_ERR(errno, "logger: closing log file");
        exit(EXIT_FAILURE);
    }

    if((temperr=pthread_mutex_unlock(&(log_queue->queue_mtx)))==ERR) {
        LOG_ERR(temperr, "logging: unlocking log queue"); exit(EXIT_FAILURE);
    }


    if(id) free(id);
    if(int_buf) free(int_buf);
    return NULL;
}


// ########## HELPER FUNCTIONS ##########

static int worker_file_open(int client_fd) {
    int i;
    int res=SUCCESS;  // will hold the result of the operation
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to open
    int path_len;   // length of the pathname
    int flags;  // will hold the operation flags the client passed
    file* new_file=NULL;    // will hold the new file's data (if it's an insert)
    file** subst_files=NULL;    // will hold the returned files (in the case of an insert in a full cache)
    unsigned subst_files_num=0;   // will hold the number of returned files

    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_open;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_open;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_open;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_open;
    if(!pathname) goto cleanup_w_open;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_open;

    // reading operation flags
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_open;
    flags=*int_buf;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_open;

    // if O_CREATE flag was used, attempt to insert the file in the cache
    if(flags==O_CREATE || flags==(O_CREATE|O_LOCK)) {
        new_file=file_create(pathname);
        if(!new_file) return ERR;   // mem err
        if((temperr=sc_cache_insert(server_cache, new_file, &subst_files))!=SUCCESS) {
            if(errno!=EFBIG && errno!=EEXIST && errno!=ENOSPC) return ERR;  // fatal error
            *int_buf=errno; // preparing the error message for the client
            writen(client_fd, (void*)int_buf, sizeof(int));
            if(new_file) {
                free(new_file->data);
                free(new_file->name);
                ll_dealloc_full(new_file->open_list);
                free(new_file);
            }
            res=ERR; goto return_expelled_files;
        }
    }

    // open the (eventually created) file
    if((temperr=sc_lookup(server_cache, pathname, OPEN_F, &client_fd, NULL, NULL, NULL, NULL))!=SUCCESS) {
        if(temperr==ERR) return ERR;    // fatal error
        *int_buf=errno;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        res=ERR; goto return_expelled_files;
    }

    // if O_LOCK flag was used, try and lock the file
    if(flags==O_LOCK || flags==(O_CREATE|O_LOCK)) {
        if((temperr=sc_lookup(server_cache, pathname, LOCK_F, &client_fd, NULL, NULL, NULL, NULL))!=SUCCESS) {
            if(temperr==ERR) return ERR;
            *int_buf=temperr;   // preparing the error message for the client
            writen(client_fd, (void*)int_buf, sizeof(int));
            res=ERR; goto return_expelled_files;
        }
    }

    if(flags==(O_CREATE|O_LOCK))new_file->f_write=1;    // enabling the writeFile operation
    *int_buf=SUCCESS;   // tell the client the operation succeeded
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) res=ERR;

    if(new_file) {LOG_OPERATION("##########\nopenCreate: client%d created file %s\n##########\n\n", client_fd, new_file->name);}
    else {LOG_OPERATION("##########\nopenFile: client%d opened file %s\n##########\n\n", client_fd, pathname);}
    if(flags==O_LOCK || flags==(O_CREATE|O_LOCK)) LOG_OPERATION("##########\nopenLock: client%d locked file %s\n##########\n\n", client_fd, pathname);


    // ##### RETURNING EXPELLED FILES #####
    return_expelled_files:

    if(subst_files) {   // if the subst files array has been allocated, some files have been expelled
        // getting the number of files removed from the cache
        for(i=0; i<(server_cache->max_file_number); i++) {
            if(subst_files[i]==NULL) break;
        }
        subst_files_num=i;
    }
    // telling the client how many files to expect
    if((writen(client_fd, (void*)&subst_files_num, sizeof(unsigned)))==ERR) res=ERR;

    // sending the substituted files back to the client
    for(i=0; i<subst_files_num; i++) {  // if there are no subst files, the cycle is skipped
        file* temp=subst_files[i];
        LOG_OPERATION("##########\nopenFile: file %s expelled due to Client%d request (read_size %lu bytes)\n##########\n\n", temp->name, client_fd, temp->file_size);
        size_t temp_path_len=strlen(temp->name);
        if((writen(client_fd, (void*)&temp_path_len, sizeof(size_t)))==ERR) res=ERR;
        if((writen(client_fd, (void*)(temp->name), temp_path_len))==ERR) res=ERR;
        if((writen(client_fd, (void*)&(temp->file_size), sizeof(size_t)))==ERR) res=ERR;
        if(temp->file_size)
            if((writen(client_fd, (void*)temp->data, (temp->file_size)))==ERR) res=ERR;
        // deallocating file from memory
        if(temp->data) free(temp->data);
        if(temp->name) free(temp->name);
        if(temp->open_list) {
            temperr=ll_dealloc_full(temp->open_list);
            if(temperr==ERR) return ERR;    // fatal error
        }
        free(temp);
    }


    if(res!=SUCCESS) goto cleanup_w_open;    // if res!=SUCCESS, something went wrong

    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(subst_files) free(subst_files);
    return SUCCESS;

// ERROR CLEANUP
cleanup_w_open:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(subst_files) free(subst_files);
    return -2;   // client-side error, server can keep working
}


static int worker_file_read(int client_fd) {
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to read
    int path_len;   // length of the pathname

    byte* data_read=NULL;   // will contain the returned file
    size_t bytes_read=0;  // will contain the size of the returned file

    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_read;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_read;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_read;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_read;
    if(!pathname) goto cleanup_w_read;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_read;

    // getting the file from the cache
    if((temperr=sc_lookup(server_cache, pathname, READ_F, &client_fd, &data_read, NULL, &bytes_read, NULL))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        goto cleanup_w_read;
    }

    // if the op succeeded, return a success message
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_read;

    // communicating the size of the file to read
    if((writen(client_fd, (void*)&bytes_read, sizeof(size_t)))==ERR) goto cleanup_w_read;

    // sending the file (ONLY IF it is not empty)
    if(bytes_read)
        if((writen(client_fd, (void*)data_read, bytes_read))==ERR) goto cleanup_w_read;
    

    LOG_OPERATION("##########\nreadFile: client%d read file %s (read_size %lu bytes)\n##########\n\n", client_fd, pathname, bytes_read);
    

    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(data_read) free(data_read);
    return SUCCESS;

// ERROR CLEANUP
cleanup_w_read:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(data_read) free(data_read);
    return -2;   // client-side error, server can keep working
}


static int worker_file_readn(int client_fd) {
    int temperr;
    int res=SUCCESS;    // temporary result of the operation
    unsigned i;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    int files_to_return=0;
    file** returned_files=NULL;     // will hold all the files to send to the client
    unsigned returned_files_num=0;  // will hold the number of files to send to the client
    unsigned long total_bytes_read=0;   // will hold the total number of bytes returned


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_readn;

    // reading the number of files to return
    if((readn(client_fd, (void*)&files_to_return, sizeof(int)))==ERR) goto cleanup_w_readn;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_readn;

    // if N was zero, return every file present
    if(!files_to_return) files_to_return=(int)(server_cache->max_file_number);

    // getting the files from the cache
    if((temperr=sc_return_n_files(server_cache, files_to_return, &returned_files))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        goto cleanup_w_readn;
    }

    // if the op succeeded, return a success message
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_readn;

    // counting how many files have been read
    if(returned_files) {   // if the returned files array has been allocated, some files have been read
        // getting the number of files retrieved
        for(i=0; i<files_to_return; i++) {
            if(returned_files[i]==NULL) break;
        }
        returned_files_num=i;
    }
    // telling the client how many files to expect
    if((writen(client_fd, (void*)&returned_files_num, sizeof(unsigned)))==ERR) res=ERR;

    // sending the retrieved files back to the client
    for(i=0; i<returned_files_num; i++) {  // if there are no subst files, the cycle is skipped
        file* temp=returned_files[i];
        total_bytes_read+=(temp->file_size);
        size_t temp_path_len=strlen(temp->name);
        if((writen(client_fd, (void*)&temp_path_len, sizeof(size_t)))==ERR) res=ERR;
        if((writen(client_fd, (void*)(temp->name), temp_path_len))==ERR) res=ERR;
        if((writen(client_fd, (void*)&(temp->file_size), sizeof(size_t)))==ERR) res=ERR;
        if(temp->file_size)
            if((writen(client_fd, (void*)temp->data, (temp->file_size)))==ERR) res=ERR;
        // deallocating file from memory
        if(temp->data) free(temp->data);
        if(temp->name) free(temp->name);
        if(temp->open_list) {
            temperr=ll_dealloc_full(temp->open_list);
            if(temperr==ERR) return ERR;    // fatal error
        }
        if(temp) free(temp);
    }
    
    if(res!=SUCCESS) goto cleanup_w_readn;
    LOG_OPERATION("##########\nreadNFiles: client%d read %u files (read_n %u read_size %lu bytes total)\n##########\n\n", client_fd, returned_files_num, returned_files_num, total_bytes_read);


    if(int_buf) free(int_buf);
    if(returned_files) free(returned_files);
    return SUCCESS;

// ERROR CLEANUP
cleanup_w_readn:
    if(int_buf) free(int_buf);
    if(returned_files) free(returned_files);
    return -2;   // client-side error, server can keep working
}

static int worker_file_write(int client_fd) {
    int i;
    int res=SUCCESS;  // will hold the result of the operation
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to write
    int path_len;   // length of the pathname
    size_t bytes_written;    // dimension of the file
    byte* data_written=NULL;     // file data
    file** subst_files=NULL;    // will hold the returned files (in the case of a write in a full cache)
    unsigned subst_files_num=0;   // will hold the number of returned files


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_write;
    if(!pathname) goto cleanup_w_write;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write;

    // reading the file size
    if((readn(client_fd, (void*)&bytes_written, sizeof(size_t)))==ERR) goto cleanup_w_write;
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write;

    // reading the file data
    data_written=(byte*)calloc(bytes_written, sizeof(byte));
    if(!data_written) return ERR;
    if((readn(client_fd, (void*)data_written, bytes_written))==ERR) goto cleanup_w_write;
    if(!data_written) goto cleanup_w_write;

    // writing the file into the cache
    if((temperr=sc_lookup(server_cache, pathname, WRITE_F, &client_fd, NULL, data_written, &bytes_written, &subst_files))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        res=ERR;
    }

    if(res==SUCCESS) { // returning a success message if there weren't error in the lookup-write op
        *int_buf=SUCCESS;
        if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) res=ERR;
    }

    LOG_OPERATION("##########\nwriteFile: client%d wrote file %s to the server (write_size %lu bytes)\n##########\n\n", client_fd, pathname, bytes_written);


    // ##### RETURNING SUBSTITUTED FILES #####

    if(subst_files) {   // if the subst files array has been allocated, 
        // getting the number of files removed from the cache
        for(i=0; i<(server_cache->max_file_number); i++) {
            if(subst_files[i]==NULL) break;
        }
        subst_files_num=i;
    }
    // telling the client how many files to expect
    if((writen(client_fd, (void*)&subst_files_num, sizeof(unsigned)))==ERR) res=ERR;

    // sending the substituted files back to the client
    for(i=0; i<subst_files_num; i++) {  // if there are no subst files, the cycle is skipped
        file* temp=subst_files[i];
        LOG_OPERATION("##########\nwriteFile: file %s expelled due to Client%d request (read_size %lu bytes)\n##########\n\n", temp->name, client_fd, temp->file_size);
        size_t temp_path_len=strlen(temp->name);
        if((writen(client_fd, (void*)&temp_path_len, sizeof(size_t)))==ERR) res=ERR;
        if((writen(client_fd, (void*)(temp->name), temp_path_len))==ERR) res=ERR;
        if((writen(client_fd, (void*)&(temp->file_size), sizeof(size_t)))==ERR) res=ERR;
        if(temp->file_size)
            if((writen(client_fd, (void*)temp->data, (temp->file_size)))==ERR) res=ERR;
        // deallocating file from memory
        if(temp->data) free(temp->data);
        if(temp->name) free(temp->name);
        if(temp->open_list) {
            temperr=ll_dealloc_full(temp->open_list);
            if(temperr==ERR) return ERR;    // fatal error
        }
        free(temp);
    }


    if(res!=SUCCESS) goto cleanup_w_write;   // if res!=SUCCESS, something went wrong

    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(data_written) free(data_written);
    if(subst_files) free(subst_files);
    return SUCCESS;

// CLEANUP SECTION
cleanup_w_write:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(data_written) free(data_written);
    if(subst_files) free(subst_files);
    return -2;
}


static int worker_file_write_app(int client_fd) {
    int i;
    int res=SUCCESS;  // will hold the result of the operation
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to write onto
    int path_len;   // length of the pathname
    size_t bytes_written;    // dimension of the write block
    byte* data_written=NULL;     // write block data
    file** subst_files=NULL;    // will hold the returned files (in the case of a write in a full cache)
    unsigned subst_files_num=0;   // will hold the number of returned files


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write_app;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write_app;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write_app;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_write_app;
    if(!pathname) goto cleanup_w_write_app;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write_app;

    // reading the write block size
    if((readn(client_fd, (void*)&bytes_written, sizeof(size_t)))==ERR) goto cleanup_w_write_app;
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_write_app;

    // reading the write block data
    data_written=(byte*)calloc(bytes_written, sizeof(byte));
    if(!data_written) return ERR;
    if((readn(client_fd, (void*)data_written, bytes_written))==ERR) goto cleanup_w_write_app;
    if(!data_written) goto cleanup_w_write_app;

    // writing the file into the cache
    if((temperr=sc_lookup(server_cache, pathname, WRITE_F_APP, &client_fd, NULL, data_written, &bytes_written, &subst_files))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        res=ERR;
    }

    if(res==SUCCESS) { // returning a success message if there weren't error in the lookup-write op
        *int_buf=SUCCESS;
        if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) res=ERR;
    }

    LOG_OPERATION("##########\nappendToFile: client%d wrote file %s to the server (write_size %lu bytes)\n##########\n\n", client_fd, pathname, bytes_written);


    // ##### RETURNING SUBSTITUTED FILES #####

    if(subst_files) {   // if the subst files array has been allocated, 
        // getting the number of files removed from the cache
        for(i=0; i<(server_cache->max_file_number); i++) {
            if(subst_files[i]==NULL) break;
        }
        subst_files_num=i;
    }
    // telling the client how many files to expect
    if((writen(client_fd, (void*)&subst_files_num, sizeof(unsigned)))==ERR) res=ERR;

    // sending the substituted files back to the client
    for(i=0; i<subst_files_num; i++) {  // if there are no subst files, the cycle is skipped
        file* temp=subst_files[i];
        LOG_OPERATION("##########\nappendToFile: file %s expelled due to Client%d request (read_size %lu bytes)\n##########\n\n", temp->name, client_fd, temp->file_size);
        size_t temp_path_len=strlen(temp->name);
        if((writen(client_fd, (void*)&temp_path_len, sizeof(size_t)))==ERR) res=ERR;
        if((writen(client_fd, (void*)(temp->name), temp_path_len))==ERR) res=ERR;
        if((writen(client_fd, (void*)&(temp->file_size), sizeof(size_t)))==ERR) res=ERR;
        if(temp->file_size)
            if((writen(client_fd, (void*)temp->data, (temp->file_size)))==ERR) res=ERR;
        // deallocating file from memory
        if(temp->data) free(temp->data);
        if(temp->name) free(temp->name);
        if(temp->open_list) {
            temperr=ll_dealloc_full(temp->open_list);
            if(temperr==ERR) return ERR;    // fatal error
        }
        free(temp);
    }


    if(res!=SUCCESS) goto cleanup_w_write_app;   // if res!=SUCCESS, something went wrong


    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(data_written) free(data_written);
    if(subst_files) free(subst_files);
    return SUCCESS;

// CLEANUP SECTION
cleanup_w_write_app:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    if(data_written) free(data_written);
    if(subst_files) free(subst_files);
    return -2;
}


static int worker_file_lock(int client_fd) {
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to lock
    int path_len;   // length of the pathname


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_lock;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_lock;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_lock;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_lock;
    if(!pathname) goto cleanup_w_lock;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_lock;

    // performing the lock operation
    if((temperr=sc_lookup(server_cache, pathname, LOCK_F, &client_fd, NULL, NULL, NULL, NULL))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        goto cleanup_w_lock;
    }

    // returning a success message
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_lock;

    LOG_OPERATION("##########\nlockFile: client%d locked file %s\n##########\n\n", client_fd, pathname);


    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return SUCCESS;

// CLEANUP SECTION
cleanup_w_lock:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return -2;
}


static int worker_file_unlock(int client_fd) {
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to unlock
    int path_len;   // length of the pathname


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_unlock;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_unlock;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_unlock;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_unlock;
    if(!pathname) goto cleanup_w_unlock;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_unlock;

    // performing the unlock operation
    if((temperr=sc_lookup(server_cache, pathname, UNLOCK_F, &client_fd, NULL, NULL, NULL, NULL))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        goto cleanup_w_unlock;
    }

    // returning a success message
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_unlock;

    LOG_OPERATION("##########\nunlockFile: client%d unlocked file %s\n##########\n\n", client_fd, pathname);


    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return SUCCESS;

// CLEANUP SECTION
cleanup_w_unlock:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return -2;
}


static int worker_file_remove(int client_fd) {
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to remove
    int path_len;   // length of the pathname


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_rm;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_rm;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_rm;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_rm;
    if(!pathname) goto cleanup_w_rm;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_rm;

    // performing the remove operation
    if((temperr=sc_lookup(server_cache, pathname, RM_F, &client_fd, NULL, NULL, NULL, NULL))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        goto cleanup_w_rm;
    }

    // returning a success message
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_rm;

    LOG_OPERATION("##########\nremoveFile: client%d removed file %s from the server\n##########\n\n", client_fd, pathname);


    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return SUCCESS;

// CLEANUP SECTION
cleanup_w_rm:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return -2;
}


static int worker_file_close(int client_fd) {
    int temperr;
    int* int_buf=(int*)malloc(sizeof(int));
    if(!int_buf) return ERR;    // errno already set by the call
    char* pathname=NULL;     // will hold the pathname of the file to close
    int path_len;   // length of the pathname


    *int_buf=SUCCESS;   // the request has been accepted
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_close;

    // reading path string length
    if((readn(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_close;
    path_len=*int_buf+1;  // getting the pathname length
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_close;

    pathname=(char*)calloc(path_len, sizeof(char)); // allocating mem for the path string
    if(!pathname) return ERR;
    memset((void*)pathname, '\0', sizeof(char)*path_len);   // setting string to 0

    // reading the pathname
    if((readn(client_fd, (void*)pathname, path_len-1))==ERR) goto cleanup_w_close;
    if(!pathname) goto cleanup_w_close;
    *int_buf=SUCCESS;   // step OK
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_close;

    // performing the close operation
    if((temperr=sc_lookup(server_cache, pathname, CLOSE_F, &client_fd, NULL, NULL, NULL, NULL))!=SUCCESS) {
        if(temperr==ERR) return ERR;
        *int_buf=temperr;   // preparing the error message for the client
        writen(client_fd, (void*)int_buf, sizeof(int));
        goto cleanup_w_close;
    }

    // returning a success message
    *int_buf=SUCCESS;
    if((writen(client_fd, (void*)int_buf, sizeof(int)))==ERR) goto cleanup_w_close;

    LOG_OPERATION("##########\ncloseFile: client%d closed (and unlocked) file %s\n##########\n\n", client_fd, pathname);


    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return SUCCESS;

// CLEANUP SECTION
cleanup_w_close:
    if(int_buf) free(int_buf);
    if(pathname) free(pathname);
    return -2;
}