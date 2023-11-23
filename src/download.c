#include <curl/curl.h>
#include <pthread.h>

#include "app.h"
#include "download.h"

static pthread_mutex_t  mutexLoader;
static int              loaderCount = 0;


static DownloadData * download_new (SoundData *data);
static int download_start (DownloadData *load);
static void * download_process (void *ptr);

int download_sound (SoundData *data) {
    DownloadData *load = download_new (data);
    if (!load)
        return FALSE;

    if (!download_start (load))
        return FALSE;

    return TRUE;
}

int download_count () {
    static int cnt;

    pthread_mutex_lock (&mutexLoader);
    cnt = loaderCount;
    pthread_mutex_unlock (&mutexLoader);

    return cnt;
}

static void download_decrement () {
    pthread_mutex_lock (&mutexLoader);
    if (loaderCount)
        loaderCount--;
    pthread_mutex_unlock (&mutexLoader);
}

static DownloadData * download_new (SoundData *data) {
    DownloadData *load = (DownloadData *) calloc (1, sizeof (DownloadData));
    if (!load) {
        selfLogErr ("Not enough memory: %m");
        return NULL;
    }
    if (!strlen (data->url)) {
        selfLogWrn ("Empty URL!");
        return NULL;
    }
    load->id = data->id;
    strcpy (load->url, data->url);
    strcpy (load->filename, data->filename);
    load->state = DL_Init;

    return load;
}

static int download_start (DownloadData *load) {
    pthread_t tid;
    int r;

    r = pthread_create (&tid, NULL, download_process, load);
    if (r) {
        selfLogErr ("Start thread error(%d): %s", r, strerror (r));
        return FALSE;
    }

    pthread_mutex_lock (&mutexLoader);
    loaderCount++;
    pthread_mutex_unlock (&mutexLoader);

    selfLogTrc ("Started load thread: %ld", tid);
    return TRUE;
}

static size_t download_write_chunk (uint8_t *ptr, size_t size, size_t nmemb, void *data) {
    FILE *fd = (FILE *) (data);
    size_t ret = fwrite (ptr, size, nmemb, fd);
    if(!ret) {
        selfLogErr ("Write chunk error(%d): %m", errno);
    }
    return ret;
}

static void * download_process (void *ptr) {
    CURL *curl;
    CURLcode ret;
    FILE *fd;
    DownloadData *data = (DownloadData *) (ptr);

    fd = fopen (data->url, "wb");
    if (!fd) {
        selfLogErr ("Cannot open file for write(%d): %m", errno);
        download_decrement ();
        return NULL;
    }

    curl = curl_easy_init();
    if(curl) {

        curl_easy_setopt(curl, CURLOPT_URL, data->url);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fd);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write_chunk);

        ret = curl_easy_perform(curl);
        (void)ret;
        // selfLogTrc ("CURL ret = %d", ret);

        /* always cleanup */
        curl_easy_cleanup(curl);

        data->state = DL_Finished;

        fclose (fd);
    }
    download_decrement ();

    return NULL;
}
