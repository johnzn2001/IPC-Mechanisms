/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
    Original author of the starter code

    Please include your name and UIN below
    Name: John Nichols
    UIN: 328001844
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include <sys/wait.h>
using namespace std;

int main(int argc, char *argv[]){
    //varible check
    int arg;
    vector<RequestChannel*> channels;
    int p = -1;
    double t = -1;
    int e = -1;
    bool c = false;
    //creation of -m/c
    char* filename = "";
    int buffercapacity = MAX_MESSAGE;
    char* serverBufferCap = "";
    string iCase = "f";
    int nchannels = 1;
    while ((arg = getopt (argc, argv, "p:t:e:c:f:m:i:")) != -1) {
        switch (arg) {
            //given person, time, ecg
            case 'p':
                p = atoi(optarg);
                break;
            case 't':
                t = atof(optarg);
                break;
            case 'e':
                e = atoi(optarg);
                break;
            case 'c':
                c = true;
                nchannels = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'm':
                buffercapacity = atoi(optarg);
                serverBufferCap = optarg;
                break;
            case 'i'://new
                iCase = optarg;
                break;
        }
    }

    //child
    if (fork() == 0){
        char* argvServer[6];
        argvServer[0] = "./server";
        if (serverBufferCap != ""){
            argvServer[1] = "-m";
            argvServer[2] = serverBufferCap;
            argvServer[3] = "-i";
            argvServer[4] = (char*) iCase.c_str();
            argvServer[5] = NULL;
        }
        else{
            argvServer[1] = "-i";
            argvServer[2] = (char*) iCase.c_str();
            argvServer[3] = NULL;
        }
        execvp("./server", argvServer);
    }

    RequestChannel* ctrlChan = NULL;
    if (iCase == "f"){
        ctrlChan = new FIFORequestChannel ("control", FIFORequestChannel::CLIENT_SIDE);
        channels.push_back(ctrlChan);
    }
    else if (iCase == "q"){
        ctrlChan = new MQRequestChannel ("control", MQRequestChannel::CLIENT_SIDE, buffercapacity);
        channels.push_back(ctrlChan);
    }
    else if (iCase == "m"){
        ctrlChan = new SHMRequestChannel ("control", SHMRequestChannel::CLIENT_SIDE, buffercapacity);
        channels.push_back(ctrlChan);
    }
    
    char buf [buffercapacity]; 
    if (c) {
        RequestChannel* chan = ctrlChan;
        for (int i = 0; i < nchannels; i++){
            MESSAGE_TYPE newChan = NEWCHANNEL_MSG;
            ctrlChan->cwrite (&newChan, sizeof (MESSAGE_TYPE));
            ctrlChan->cread(buf, buffercapacity);
            char* chan2Name = buf;
            if (iCase == "f"){
                chan = new FIFORequestChannel (chan2Name, FIFORequestChannel::CLIENT_SIDE);
                channels.push_back(chan);
            }
            else if (iCase == "q"){
                chan = new MQRequestChannel (chan2Name, MQRequestChannel::CLIENT_SIDE, buffercapacity);
                channels.push_back(chan);
            }
            else if (iCase == "m"){
                chan = new SHMRequestChannel (chan2Name, SHMRequestChannel::CLIENT_SIDE, buffercapacity);
                channels.push_back(chan);
            }
        }
    }
    //1
    if (p > 0 && t >= 0 && e > 0){
        for (int i = 0; i < channels.size(); i++){
            RequestChannel* curr_channel = channels.at(i);
            datamsg* x = new datamsg (p, t, e);
            curr_channel->cwrite (x, sizeof (datamsg));
            int nbytes = curr_channel->cread (buf, buffercapacity);
            double outR = *(double *) buf;
            cout << curr_channel->name() << " reply (p=" << p << " t=" << t << " e=" << e << "): " << outR << endl;
            delete x;
        }
    }
    //1000
    if (p > 0 && t == -1 && e > 0){
        fstream tOne, tTwo;
        tOne.open("received/x1.csv", fstream::out);
        tTwo.open("received/x1Time.csv", fstream::out);        
        struct timeval tCurrent;
        double t;
        int indexChan = 0;
        struct timeval bTime, aTime;
        gettimeofday(&bTime, NULL);
        int timeB = bTime.tv_sec * 1000000 + bTime.tv_usec;
        for(int i = 0; i < 1000; i++){
            if (i == 0) {
                t = 0;
            }
            else{
                t = t + 0.004;
            }
            RequestChannel* curr_channel = channels.at(indexChan);
            datamsg* x = new datamsg (p, t, e);
            curr_channel->cwrite (x, sizeof (datamsg));
            curr_channel->cread (buf, buffercapacity);
            double outR = *(double *) buf;
            tOne << outR << "\n";
            gettimeofday(&tCurrent, NULL);
            int currentTimeUs = tCurrent.tv_sec * 1000000 + tCurrent.tv_usec;
            tTwo << currentTimeUs << "\n";
            delete x;

            if (indexChan == channels.size() - 1){
                indexChan = 0;
            }
            else{
                indexChan++;
            }
        }

        gettimeofday(&aTime, NULL);
        int timeA = aTime.tv_sec * 1000000 + aTime.tv_usec;
        cout << "1K data points time: " << timeA - timeB << " us\n";
        tOne.close();
        tTwo.close();
        cout << "data written to x1.csv\n";
    }
    //rework efficient
    if (filename != ""){
        string fName = filename;
        filemsg fm(0, 0);
        char buffF [sizeof(filemsg) + fName.size() + 1];
        memcpy (buffF, &fm, sizeof(filemsg));
        memcpy (buffF + sizeof(filemsg), fName.c_str(), fName.size() + 1);
        ctrlChan->cwrite (buffF, sizeof(filemsg) + fName.size() + 1);
        __int64_t lenF;
        ctrlChan->cread(&lenF, sizeof(__int64_t));
        __int64_t replaceByte = lenF;
        ofstream outputFile("received/" + fName, fstream::out);
        struct timeval bTime, aTime;
        gettimeofday(&bTime, NULL);
        int timeB = bTime.tv_sec * 1000000 + bTime.tv_usec;
        int indexChan = 0;
        while (replaceByte > 0){
            RequestChannel* curr_channel = channels.at(indexChan);
            __int64_t offset = lenF - replaceByte;
            int length = buffercapacity;
            if (replaceByte >= buffercapacity){
                replaceByte = replaceByte - buffercapacity;
            }
            else{
                length = replaceByte;
                replaceByte = 0;
            }
            filemsg fm(offset, length);
            char buffF [sizeof(filemsg) + fName.size() + 1];
            memcpy (buffF, &fm, sizeof(filemsg));
            memcpy (buffF + sizeof(filemsg), fName.c_str(), fName.size() + 1);
            curr_channel->cwrite (buffF, sizeof(filemsg) + fName.size() + 1);
            char outputBuffer[length]; 
            curr_channel->cread(outputBuffer, length);
            outputFile.write((char*) &outputBuffer, sizeof(outputBuffer));
            if (indexChan == channels.size() - 1){
                indexChan = 0;
            }
            else{
                indexChan++;
            }
        }
        gettimeofday(&aTime, NULL);
        int timeA = aTime.tv_sec * 1000000 + aTime.tv_usec;
        cout << "Transfer time of " << fName << " with buffer of " << buffercapacity << ": " << timeA - timeB << " us\n";
        outputFile.close();
    }
    
    // exit
    MESSAGE_TYPE m = QUIT_MSG;
    for (int i = 0; i < channels.size(); i++){
        RequestChannel* curr_channel = channels.at(i);
        curr_channel->cwrite (&m, sizeof (MESSAGE_TYPE));
        delete curr_channel;
    }
    wait(&arg);
}