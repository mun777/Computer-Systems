#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "FIFORequestChannel.h"
using namespace std;
// ecgno to use for datamsgs
#define EGCNO 1

using namespace std;
// p_no from message and reponse from server
struct histogram
{
    int person;
    double ECG;
};
void patient_thread_function (int p_no, BoundedBuffer* request, int n, int buffsize) {
    // functionality of the patient threads

    // take a patient number p_no
    // for n requests, produce a datasmg (num, time, ECGNO) and push to request_buffer
    //  -time dependant on current requests
    //  -at 0 -> time - 0.00; at 1-> time = 0.004; at 2-> time - 0.008;... find function
    for(int i=0; i< n; i++)
    {
        char buf[buffsize];
        double time = double(i*0.004);
        datamsg ECG1(p_no,time, EGCNO);
        memcpy(&buf,&ECG1,sizeof(datamsg));
        request->push(buf,sizeof(datamsg));
    }

}

void file_thread_function (string filename, BoundedBuffer* requestbuf, FIFORequestChannel* pipe, int buffsize) {
    // functionality of the file thread

    // file size
    // while offset < file_size, produce a filemsg(offset, m) + filename and push to request_buffer
    // open output file; allocate the memory fseek: close buffer
    //  -incrementing offset: and be careful with the final message

    char buf[buffsize];
    filemsg msg = filemsg(0,0);
    
    memcpy(buf, &msg, sizeof(filemsg));
	strcpy(buf + sizeof(filemsg), filename.c_str());
    int len = sizeof(filemsg) + (filename.size() + 1);
    pipe->cwrite(buf, len);
    int64_t filelen = 0;
    pipe->cread(&filelen, sizeof(int64_t));
    //int numloops = ceil((double) filelen/buffsize);
    //creating file
    string openedfile = "./received/" + filename;
    
    FILE* fp = fopen(openedfile.c_str(),"w+");
    fseek(fp,filelen, SEEK_SET);
    fclose(fp);
    
    auto remain = filelen;
    filemsg*fm = (filemsg*) buf;
    while(remain>0)
    {
        fm->length = min(buffsize, (int) remain);
        int len = sizeof(filemsg)+filename.size()+1;
        requestbuf->push((char*)fm,len);
        fm->offset += fm->length;
        remain -= fm->length;
    }

}

void worker_thread_function (BoundedBuffer*request, BoundedBuffer*response, FIFORequestChannel* pipe, int buffsize) {
    // functionality of the worker threads
    // forever loop
    // pop message from the request _buffer
    // view line 120 in server (process_request) for how to decide current message

    // if DATA: 
    //send message across FIFO and collect response

    //  -create pair of p_no from message and reponse from server
    //  -push pair to the reponse_buffer

    // if FILE:
    // collect the filename from the message
    //  -open the file in update mode
    //  -fseek(SEEK_SET) to offset of the filemsg
    //  -write the buffer free from the server

    while(true){
        char main_buff[buffsize];
        char* bufhold = main_buff;
        int size = request->pop(main_buff, buffsize);
        MESSAGE_TYPE m = *((MESSAGE_TYPE*) main_buff);
        if(m == DATA_MSG){
            datamsg* buf = (datamsg*) bufhold;
            double reply;
            pipe->cwrite(buf,sizeof(datamsg));
            pipe->cread(&reply, sizeof(double));
            histogram h = {((datamsg*)buf)->person,reply};

            response->push((char*)&h,sizeof(h));
        }
        if(m == FILE_MSG)
        {
            filemsg* message = (filemsg*) main_buff;
            bufhold += sizeof(filemsg);
            string fname = string(bufhold);
            string output = "./received/" + fname;
            pipe->cwrite(main_buff,size);
            char* rec[buffsize];
            memset(rec,0,buffsize);
            pipe->cread(rec, buffsize);

            FILE *fp = fopen(output.c_str(),"r+");
            if(fp == nullptr)
            {
                break;
            }
            int offset = message->offset;
            fseek(fp,offset,SEEK_SET);
            fwrite(rec,message->length,1,fp);
            fclose(fp);

        }

        if(m == QUIT_MSG)
        {
            pipe->cwrite(&m, sizeof(QUIT_MSG));
            delete pipe;
            break;
        }
    }
    
}

void histogram_thread_function (BoundedBuffer* response, HistogramCollection* hist) {
    // functionality of the histogram threads
    //
    // forever loop
    // pop response from the response_buffer
    // call HC::update (response-> p_no, resp->double)
    while(true){
        histogram data;
        
        response->pop(reinterpret_cast<char*>(&data), sizeof(histogram));// into buffer

        if(data.person < 0){
            break;
        }
        hist->update(data.person, data.ECG);
    }
    
    
}
//function for making channels
FIFORequestChannel *newshit(FIFORequestChannel *chan)
{
    char buffname[1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    chan->cwrite(&m,sizeof(MESSAGE_TYPE));
    chan->cread(&buffname,sizeof(buffname));
    FIFORequestChannel *new_chan = new FIFORequestChannel(buffname, FIFORequestChannel::CLIENT_SIDE);
    return new_chan;
}
int main (int argc, char* argv[]) {
    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 20;		// default capacity of the request buffer (should be changed)
	int m = MAX_MESSAGE;	// default capacity of the message buffer
	string f = "";	// name of file to be transferred
    
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
		}
	}
    
	// fork and exec the server
    int pid = fork();
    if (pid == 0) {
        execl("./server", "./server", "-m", (char*) to_string(m).c_str(), nullptr);
    }
    
	// initialize overhead (including the control channel)
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;
    vector<FIFORequestChannel*> worker_chan;
    // array or vector producer threads (if data, p elements: if file, 1 element)
    // array of fifo (w elements)
    // array of worker threads (w elements )
    // array of histogram threads (if data, h elements: if file, zero elements)

    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
    //for worker, adding to channel vector
    for(int i=0; i <w; i++)
    {
        FIFORequestChannel *new_chan = newshit(chan);
        worker_chan.push_back(new_chan);

    }
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    /* create all threads here */
    thread *patients = new thread[p];
    thread *histograms = new thread[h];
    thread *workers = new thread[w];
    thread file_thread;
    //if data:
    //  -create p patient_threads (store in producer thread)
    //  -create w worker_threads (store in worker array)
    //      ->create channel (store fifo array)
    //  -create h histogram_threads ( store in hist array)
    // if file:
    //  -create 1 file_thread ( store producer array)
    //  -create w worker_threads (store in worker array)
    //      ->create channel (store fifo array)  
    if(f!="")
    {
        file_thread = thread(file_thread_function,f,&request_buffer,chan,m);

    }
    else{
        for(int i=0; i < p; i++)
        {
            patients[i] = thread(patient_thread_function, i+1, &request_buffer, n, m);
        }
        for(int i=0; i < h; i++)
        {
            histograms[i] = thread(histogram_thread_function, &response_buffer, &hc);
        }
    }

    for(int i=0; i < w; i++)
    {
        workers[i] = thread(worker_thread_function,&request_buffer,&response_buffer,worker_chan[i],m);

    }
    // if data:
    //  -create p patient_threads
    // if file:
    //  -create 1 file_thread
    // create w worker_threads
    // - create w channels
    //if data:
    // -create h hist


	/* join all threads here */
    // iterate over all thread arrays, calling join
    //  -order is important: producers before consumers
    // join patient worker histogram
    if(f!="")
    {
        file_thread.join();
    }
    else{
        for(int i=0; i < p; i++)
        {
            patients[i].join();
        }
    }
    //delete worker, sending message
    
    MESSAGE_TYPE quit = QUIT_MSG;
    for(int i=0; i<w; i++)
    {
        request_buffer.push((char *) &quit, sizeof (MESSAGE_TYPE));
    }
    
    for(int i=0; i < w; i++)
    {
        workers[i].join();
    }
    if(f=="")
    {
        for(int i=0; i<h; i++)
        {
            histogram quit_sig = {-1,-1.0};
            response_buffer.push((char *) &quit_sig, sizeof (MESSAGE_TYPE));
        }
        for(int i=0; i < h; i++)
        {
            histograms[i].join();
       }
    }
    delete []patients;
    delete []histograms;
    delete []workers;



	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
    // quit and close all channels in FIFO array
	// quit and close control channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!" << endl;
    delete chan;

	// wait for server to exit
	wait(nullptr);
}
