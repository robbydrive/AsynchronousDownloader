#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <curl/multi.h>

using namespace std;

size_t write_callback(char *ptr, size_t size, size_t nmemb, int *userdata)
{
    *userdata += size * nmemb;
    return size * nmemb;
}

int main(int argc, char * argv[])
{
    CURLM *mh;
    CURLMcode resCode;
    CURLMsg *message;
    char *url;
    int runningTransfers = -1, maxfd, msgsLeft, *sizePtr;
    long timeout;
    fd_set readfds, writefds, excfds;
    struct timeval timeout_struct;

    curl_global_init(CURL_GLOBAL_ALL);
    mh = curl_multi_init();

    if (argc == 1)
    {
        cout << "Wrong usage: asyncDownloader {link 1} [link 2] ... [link n]" << endl;
        return -1;
    }

    int results[argc-1] = { 0 };

    for (size_t i = 1; i < argc; ++i)
    {
        CURL *eh = curl_easy_init();
        curl_easy_setopt(eh, CURLOPT_URL, argv[i]);
        curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(eh, CURLOPT_PRIVATE, &results[i-1]);
        curl_easy_setopt(eh, CURLOPT_WRITEDATA, &results[i-1]);
        curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L);
        cout << "arguments set" << endl;
        curl_multi_add_handle(mh, eh);
    }

    cout << "Link\tSize" << endl;

    while (runningTransfers)
    {
        curl_multi_perform(mh, &runningTransfers);

        if (runningTransfers) {
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_ZERO(&excfds);

            if ((resCode = curl_multi_fdset(mh, &readfds, &writefds, &excfds, &maxfd))) {
                cout << "Problem with fd sets, error code - " << resCode;
                return -1;
            }

            if ((resCode = curl_multi_timeout(mh, &timeout))) {
                cout << "Problem with setting time out, error code - " << resCode;
                return -1;
            }

            // If timeout == -1, there is no stored timeout value and it should be set manually
            if (timeout == -1)
                timeout = 100;

            // If maxfd == -1, then it is necessary to wait until libcurl will be ready for select
            if (maxfd == -1)
                sleep((unsigned int) timeout / 1000);
            else {
                timeout_struct.tv_sec = timeout / 1000;
                timeout_struct.tv_sec = (timeout % 1000) * 1000;

                if (select(maxfd + 1, &readfds, &writefds, &excfds, &timeout_struct) < 0) {
                    cout << "Problem with select, error code - " << errno
                         << "; description: " << strerror(errno) << endl;
                    return -1;
                }
            }
        }

        while ((message = curl_multi_info_read(mh, &msgsLeft)))
        {
            CURL *eh = message->easy_handle;
            curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &url);
            if (message->msg == CURLMSG_DONE && message->data.result == CURLE_OK)
            {
                curl_easy_getinfo(eh, CURLINFO_PRIVATE, &sizePtr);
                cout << url << "\t" << *sizePtr << endl;
            }
            else if (message->msg == CURLMSG_DONE)
            {
                cout << url << "\t" << "Error CURLCode " << message->data.result << endl;
            }
            else
            {
                cout << url << "\t" << "Error CURLMsg code " << message->msg << endl;
            }
            curl_multi_remove_handle(mh, eh);
            curl_easy_cleanup(eh);
        }
    }

    curl_global_cleanup();
    return 0;
}