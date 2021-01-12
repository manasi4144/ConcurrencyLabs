/*
 * The code is derived from cURL example and paster.c base code.
 * The cURL example is at URL:
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * Copyright (C) 1998 - 2018, Daniel Stenberg, <daniel@haxx.se>, et al..
 *
 * The xml example code is 
 * http://www.xmlsoft.org/tutorial/ape.html
 *
 * The paster.c code is 
 * Copyright 2013 Patrick Lam, <p23lam@uwaterloo.ca>.
 *
 * Modifications to the code are
 * Copyright 2018-2019, Yiqing Huang, <yqhuang@uwaterloo.ca>.
 * 
 * This software may be freely redistributed under the terms of the X11 license.
 */

/** 
 * @file main_wirte_read_cb.c
 * @brief cURL write call back to save received data in a user defined memory first
 *        and then write the data to a file for verification purpose.
 *        cURL header call back extracts data sequence number from header if there is a sequence number.
 * @see https://curl.haxx.se/libcurl/c/getinmemory.html
 * @see https://curl.haxx.se/libcurl/using/
 * @see https://ec.haxx.se/callback-write.html
 */ 

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/multi.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <search.h>
#include <libxml/uri.h>

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */
#define SEED_URL "http://ece252-1.uwaterloo.ca/lab5"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define CT_PNG  "image/png"
#define CT_HTML "text/html"
#define CT_PNG_LEN  9
#define CT_HTML_LEN 9

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

int parts_count = 0;

typedef struct node {
    char* url;
    struct node* next;
} node;

typedef struct userdata {
    char* url;
    RECV_BUF recv_buf;
} USERDATA;


node* list_head;
node* list_tail;

char png_list[100][256];

htmlDocPtr mem_getdoc(char *buf, int size, const char *url);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath);
int find_http(char *fname, int size, int follow_relative_links, const char *base_url);
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
void cleanup(CURL *curl, RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
CURL *easy_handle_init(RECV_BUF *ptr, const char *url, CURLM *cm);
int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf);


htmlDocPtr mem_getdoc(char *buf, int size, const char *url)
{
    int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | \
               HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    htmlDocPtr doc = htmlReadMemory(buf, size, url, NULL, opts);
    
    if ( doc == NULL ) {
       // fprintf(stderr, "Document not parsed successfully.\n");
        return NULL;
    }
    return doc;
}

xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath)
{
	
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL) {
       // printf("Error in xmlXPathNewContext\n");
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL) {
       // printf("Error in xmlXPathEvalExpression\n");
        return NULL;
    }
    if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
        xmlXPathFreeObject(result);
       // printf("No result\n");
        return NULL;
    }
    return result;
}

int find_http(char *buf, int size, int follow_relative_links, const char *base_url)
{

    int i;
    htmlDocPtr doc;
    xmlChar *xpath = (xmlChar*) "//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    xmlChar *href;
		
    if (buf == NULL) {
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);
    result = getnodeset (doc, xpath);
    if (result) {
        nodeset = result->nodesetval;
        for (i=0; i < nodeset->nodeNr; i++) {
            href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
            if ( follow_relative_links ) {
                xmlChar *old = href;
                href = xmlBuildURI(href, (xmlChar *) base_url);
                xmlFree(old);
            }
            if ( href != NULL && !strncmp((const char *)href, "http", 4) ) {
                node* n = malloc( sizeof( node ));
                n->url = (char *)href;
                //printf("href is: %s\n", n->url);
                n->next = NULL;
                if (list_head == NULL)
                {
                    list_head = n;
                    //printf("href head is: %s\n", list_head->url);
                    list_tail = n;
                }
                else
                {
                    list_tail->next = n;
                    list_tail = list_tail->next;
                    //printf("href tail is : %s\n", list_tail->url);
                }
                
               // printf("href should be: %s\n", href); //finding other URLS as well?
            }
            //xmlFree(href);
        }
        xmlXPathFreeObject (result);
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}
/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structures
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;

#ifdef DEBUG1_
    //printf("%s", p_recv); //prints out the header information 
#endif /* DEBUG1_ */
    if (realsize > strlen(ECE252_HEADER) &&\
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be positive */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

void cleanup(CURL *curl, RECV_BUF *ptr)
{
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        recv_buf_cleanup(ptr);
}
/**
 * @brief output data in memory to a file
 * @param path const char *, output file path
 * @param in  void *, input data to be written to the file
 * @param len size_t, length of the input data in bytes
 */

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
       // fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
      //  fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
      //  fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}

/**
 * @brief create a curl easy handle and set the options.
 * @param RECV_BUF *ptr points to user data needed by the curl write call back function
 * @param const char *url is the target url to fetch resoruce
 * @return a valid CURL * handle upon sucess; NULL otherwise
 * Note: the caller is responsbile for cleaning the returned curl handle
 */

CURL *easy_handle_init(RECV_BUF *ptr, const char *url, CURLM *cm)
{
    CURL *curl_handle = NULL;

    if ( ptr == NULL || url == NULL) {
        return NULL;
    }

    /* init user defined call back function buffer */
    if ( recv_buf_init(ptr, BUF_SIZE) != 0 ) {
        return NULL;
    }
    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
      //  fprintf(stderr, "curl_easy_init: returned NULL\n");
        return NULL;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_PRIVATE, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)ptr);

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)ptr);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ece252 lab4 crawler");

    /* follow HTTP 3XX redirects */
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    /* continue to send authentication credentials when following locations */
    curl_easy_setopt(curl_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    /* max numbre of redirects to follow sets to 5 */
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    /* supports all built-in encodings */ 
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    /* Max time in seconds that the connection phase to the server to take */
    //curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
    /* Max time in seconds that libcurl transfer operation is allowed to take */
    //curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    /* Time out for Expect: 100-continue response in milliseconds */
    //curl_easy_setopt(curl_handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);

    /* Enable the cookie engine without reading any initial cookies */
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    /* allow whatever auth the proxy speaks */
    curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    /* allow whatever auth the server speaks */
    curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_multi_add_handle(cm, curl_handle);
    return curl_handle;
}

int process_html(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    char fname[256];
    int follow_relative_link = 1;
    char *url = NULL; 
    pid_t pid =getpid();

    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
    find_http(p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url); 
    sprintf(fname, "./output_%d.html", pid);
    return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
}

int is_png_file(unsigned char* buf)
{
	unsigned char signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

	for (int i = 0; i < 8; i++)
	{
		if (signature[i] != buf[i])
        {
            //printf("Not a PNG file \n");
                return -1;
        }
	}
	return 0;
}
int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    pid_t pid =getpid();
    char fname[256];
    char *eurl = NULL;          /* effective URL */
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);
    if ( eurl != NULL) {
       // printf("The PNG url is: %s\n", eurl);
    }

    int is_png = is_png_file((unsigned char*)p_recv_buf->buf);
    if (is_png == 0)
    {  
        int found = 0;

        for (int i = 0; i < parts_count; i++)
        {
            if (strcmp(eurl, png_list[i]) == 0) //url found
            {
                found = 1;
            }
        }
        if (found == 0) //url not found in png_url list
        {
          //  printf("url not found: %s\n ", eurl);
            strcpy(png_list[parts_count], eurl);
            parts_count++;
            FILE *fp;
            fp = fopen("png_urls.txt", "a");
            if (fp == NULL) {
                perror("fopen");
                return -2;
            }
            fprintf(fp,"%s\n", eurl);
            fclose(fp);
            sprintf(fname, "./output_%d_%d.png", p_recv_buf->seq, pid);
            return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
        }
      //  printf("url found: %s\n ", eurl);

    }
    return 0;
}

/**
 * @brief process the download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */

int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    CURLcode res;
    char fname[256];
    pid_t pid =getpid();
    long response_code;

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if ( res == CURLE_OK ) {
	    //printf("Response code: %ld\n", response_code);
    }

    if ( response_code >= 400 ) { 
    //	fprintf(stderr, "Error.\n");
        return 1;
    }

    char *ct = NULL;
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if ( res == CURLE_OK && ct != NULL ) {
    	//printf("Content-Type: %s, len=%ld\n", ct, strlen(ct));
    } else {
     //   fprintf(stderr, "Failed obtain Content-Type\n");
        return 2;
    }

    if ( strstr(ct, CT_HTML) ) {
        return process_html(curl_handle, p_recv_buf);
    } else if ( strstr(ct, CT_PNG) ) {
        return process_png(curl_handle, p_recv_buf);
    } else {
        sprintf(fname, "./output_%d", pid);
    }

    return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
}

int single_thread(char* url, int num_png, char* log, int num_connect)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    hcreate(4096);
    //int res1;
    ENTRY new, *ep;
    USERDATA data[num_connect];
  //  printf("parts_count is %d\n ", parts_count);
    //RECV_BUF recv_buf;
    CURL* handle;
    CURLM *cm = NULL;
    CURL *eh = NULL;
    CURLMsg *msg = NULL;
    CURLcode return_code=0;
    int still_running=0, msgs_left=0;
    int http_status_code;
    char *szUrl;
        //REC_BUF *type = malloc(num_connect*sizeof(recv_buf));
    cm = curl_multi_init();
    data[0].url = url;
  //  printf("data.url = %s \n", data[0].url);
    handle = easy_handle_init(&data[0].recv_buf, url, cm);
    if ( handle == NULL ) {
            //    fprintf(stderr, "Curl initialization failed. Exiting...\n");
                curl_global_cleanup();
                abort();
            }
    curl_multi_perform(cm, &still_running);

    while(1)
    {
        do {
            int numfds=0;
            int res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
            if(res != CURLM_OK) {
             //   fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
                return EXIT_FAILURE;
        }
        curl_multi_perform(cm, &still_running);

        } while(still_running);

        while ((msg = curl_multi_info_read(cm, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                eh = msg->easy_handle;

                return_code = msg->data.result;
                if(return_code!=CURLE_OK) {
                  //  fprintf(stderr, "CURL error code: %d\n", msg->data.result);
                    curl_easy_getinfo(eh, CURLINFO_PRIVATE, &szUrl);
                    if(strcmp(log, "none") != 0)
                    {
                        FILE *fp;
                        fp = fopen(log, "a");
                        if (fp == NULL) {
                            perror("fopen");
                            return -2;
                        }
                        fprintf(fp,"%s\n", szUrl);
                        fclose(fp);
                    }
                    continue;
                }

                // Get HTTP status code
                http_status_code=0;
                szUrl = NULL;

                curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
                curl_easy_getinfo(eh, CURLINFO_PRIVATE, &szUrl);

                if(http_status_code==200) {
                  //  printf("200 OK for %s\n", szUrl);
                    int i;
                    for (i = 0; i < num_connect; i++)
                    {
                      //  printf("data.url = %s \n", data[i].url);
                        if (strcmp(szUrl,data[i].url) == 0)
                        {
                            break;
                        }
                    }
                    process_data(eh, &data[i].recv_buf);  
                    if(strcmp(log, "none") != 0)
                    {
                        FILE *fp;
                        fp = fopen(log, "a");
                        if (fp == NULL) {
                            perror("fopen");
                            return -2;
                        }
                        fprintf(fp,"%s\n", szUrl);
                        fclose(fp);
                    }
                    if (parts_count == num_png)
                    {
                        curl_multi_cleanup(cm);
                        return 0;
                    } 
                
                } else {
                  //  fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
                }
                    
                curl_multi_remove_handle(cm, eh);
                curl_easy_cleanup(eh);
            }
            else {
              //  fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
            }
        }
        int done = 0;
        //selecting the next batch of connections
        for (int i = 0; i < num_connect; i++)
        {
            if (list_head == NULL)
            {
                if (i == 0)
                {
                    curl_multi_cleanup(cm);
                    return 0;
                }
                else
                {
                    break;
                }
                
            }
            if (list_head->next == NULL)
            {
                if (i == 0)
                {
                    curl_multi_cleanup(cm);
                    return 0;
                }
                else
                {
                    break;
                }
                
            }
            int j = 0;
            while(j == 0)
            {
                node *temp = list_head;
                list_head = list_head->next;
                if (list_head == NULL)
                {
                    done = 1;
                    i = num_connect;
                    break;
                }
                url = list_head->url;
             //   printf("url is %s\n", url);
                free(temp);  
                new.key = url;
                new.data = NULL;
                ep = hsearch(new, FIND);
                if (ep == NULL) //url not found in visited list
                {
                    j = 1;
                }
            }
            if (done == 0)
            {
                data[i].url = url;
                ep = hsearch(new, ENTER);
                if (ep == 0)
                {
                //  fprintf(stderr, "hsearch enter error\n");
                }
                easy_handle_init(&data[i].recv_buf, url, cm);
            }
            
        }
        curl_multi_perform(cm, &still_running);
 
    }
    
    curl_multi_cleanup(cm);

}
    
int main( int argc, char** argv ) 
{
    char url[256];
    int num_connections = 1;
    int num_png = 50;
    char* log = "none";
    char *str = "option requires an argument";
    int c;

    strcpy(url, SEED_URL); 

 while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
        switch (c) {
        case 't':
	    num_connections = strtoul(optarg, NULL, 10);
	   // printf("option -t specifies a value of %d.\n", num_connections);
	    if (num_connections <= 0) {
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                return -1;
            }
            break;
        case 'm':
            num_png = strtoul(optarg, NULL, 10);
	        //printf("option -m specifies a value of %d.\n", num_png);
            if (num_png <= 0) {
                fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
                return -1;
            }
            break;
        case 'v':
            log = optarg;
	   // printf("option -v specifies a value of %s.\n", log);
            break;
        default:
            return -1;
        }
    }

	if (optind < argc)
{
	strcpy(url, argv[optind]);
}

    double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;
    
    //printf("num_png is %d \n", num_png);
    single_thread(url, num_png, log, num_connections);

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec / 1000000.;
	
    printf("findpng3 execution time: %lf seconds\n", times[1] - times[0]);

    return 0;
}   


