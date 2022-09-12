// your PA3 BoundedBuffer.cpp code here
#include "BoundedBuffer.h"
#include <mutex>
#include <vector>
#include <cstring>
#include <iostream>
#include <assert.h>
using namespace std;


BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    // 2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
    // 3. Then push the vector at the end of the queue
    // 4. Wake up threads that were waiting for push
    vector<char> mess(msg,msg+size);
    unique_lock<std::mutex> buf(lock);

   

    pushmsg.wait(buf,[this]{return (int) q.size()<cap;});
    q.push(mess);

    buf.unlock();
    popmsg.notify_one();
    /*
    unique_lock<mutex>;
    wait(mutex,cap_check());
    cap_check();
    */
}

int BoundedBuffer::pop (char* msg, int size) {
    // 1. Wait until the queue has at least 1 item
    // 2. Pop the front item of the queue. The popped item is a vector<char>
    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    // 4. Wake up threads that were waiting for pop
    // 5. Return the vector's length to the caller so that they know how many bytes were popped
    unique_lock<std::mutex> buf(lock);

    popmsg.wait(buf,[this] {return !q.empty();});

    vector<char> mess = q.front();
    q.pop();

    buf.unlock();
 

    assert((int) mess.size() <= size);
    memcpy(msg,mess.data(),mess.size());


    pushmsg.notify_one();

    return mess.size();
}

size_t BoundedBuffer::size () {
    return q.size();
}