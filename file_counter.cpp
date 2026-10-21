#include <iostream>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <vector>
using namespace std;
int badF = 0;
int directories = 0;
int regularF = 0;
int specialF = 0;
long long rfbytes = 0;
int textF = 0;
long long tfBytes = 0;
pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;

bool isTextFile(const char *filename)
{
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
        return false;
    const int BUFFER_SIZE = 9999;
    unsigned char buffer[BUFFER_SIZE];

    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        for (ssize_t i = 0; i < bytesRead; i++)
        {
            if (!isprint(buffer[i]) && !isspace(buffer[i]))
            {
                close(fd);
                return false;
            }
        }
    }
    close(fd);
    if (bytesRead < 0)
        return false;

    return true;
}

void processFile(const char *filename)
{
    struct stat fileInfo;

    if (stat(filename, &fileInfo) < 0)
    {
        pthread_mutex_lock(&sMutex);
        badF++;
        pthread_mutex_unlock(&sMutex);
        return;
    }
    if (S_ISDIR(fileInfo.st_mode))
    {
        pthread_mutex_lock(&sMutex);
        directories++;
        pthread_mutex_unlock(&sMutex);

        return;
    }
    if (S_ISREG(fileInfo.st_mode))
    {
        bool text = isTextFile(filename);

        pthread_mutex_lock(&sMutex);

        regularF++;
        rfbytes += fileInfo.st_size;

        if (text)
        {
            textF++;
            tfBytes += fileInfo.st_size;
        }
        pthread_mutex_unlock(&sMutex);
        return;
    }
    pthread_mutex_lock(&sMutex);
    specialF++;
    pthread_mutex_unlock(&sMutex);
}

void *worker(void *arg)
{
    char *filename = static_cast<char *>(arg);
    processFile(filename);
    delete[] filename;
    return nullptr;
}

void printResults()
{
    cout << "Bad Files: " << badF << endl;
    cout << "Directories: " << directories << endl;
    cout << "Regular Files: " << regularF << endl;
    cout << "Special Files: " << specialF << endl;
    cout << "Regular File Bytes: " << rfbytes << endl;
    cout << "Text Files: " << textF << endl;
    cout << "Text File Bytes: " << tfBytes << endl;
}



void serial()
{
    char filename[9999];

    while (cin.getline(filename, sizeof(filename)))
    {
        processFile(filename);
    }

    printResults();
}
void multithreaded(int maxThreads)
{
    vector<pthread_t> threads;

    char filename[9999];

    while (cin.getline(filename, sizeof(filename)))
    {
        int length = strlen(filename);
        char *threadFilename = new char[length + 1];
        strcpy(threadFilename, filename);
        pthread_t thread;
        if (pthread_create(&thread, nullptr, worker, threadFilename) != 0)
        {
            delete[] threadFilename;
            processFile(filename);
            continue;
        }
        threads.push_back(thread);
        if ((int)threads.size() >= maxThreads)
        {
            pthread_join(threads.front(), nullptr);
            threads.erase(threads.begin());
        }
    }
    for (pthread_t thread : threads)
    {
        pthread_join(thread, nullptr);
    }

    printResults();
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        serial();
    }
    else if (argc == 3 && strcmp(argv[1], "thread") == 0)
    {
        int maxThreads = atoi(argv[2]);
        if (maxThreads < 1 || maxThreads > 15)
        {
            cerr << "Number of threads must be between 1 and 15." << endl;
            return 1;
        }
        multithreaded(maxThreads);
    }
    else
    {
        cerr << "Usage:" << endl;
        cerr << "  ./proj4" << endl;
        cerr << "  ./proj4 thread <number-of-threads>" << endl;
        return 1;
    }
    return 0;
}
