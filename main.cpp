/**
 * Parallel listing of a directory
 */
#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <future>
#include <condition_variable>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

template<class T>
class MsgQueue {
    std::mutex _mutex;
    std::condition_variable cond;
    std::deque<T> _Queue;

public:
    void send(T && msg){
        std::lock_guard<std::mutex> lck(_mutex);
        _Queue.push_back(msg);
        cond.notify_one();
    }
    T receive() {
        std::unique_lock<std::mutex> lck(_mutex);
        cond.wait(lck, [this](){return !this->_Queue.empty();});
        T res = _Queue.front();
        _Queue.pop_front();
        return res;
    }
};

struct DirServer {
    void operator()(MsgQueue<path>& dirQueue, MsgQueue<std::string>& fileQueue){
        for(;;){
            path dir = dirQueue.receive();
            // Break the loop when received an empty dir
            if(dir == "") break;

            for(auto it=directory_iterator(dir);it!=directory_iterator();++it){
                if(is_directory(*it)){
                    dirQueue.send(*it);
                } else {
                    fileQueue.send(it->filename());
                }
            }
        }
    }
};

struct PrintServer {
    void operator()(MsgQueue<std::string>& fileQueue){
        for(;;){
            std::string filename = fileQueue.receive();
            std::cout << filename << std::endl;
        }
    }
};

void
listTree(path && startingPath){

    // Fire threads
    const int numThreads = 4;
    std::vector<std::future<void>> futures;
    MsgQueue<path> dirQueue;
    MsgQueue<std::string> fileQueue;

    dirQueue.send(std::move(startingPath));

    for(int i=0;i<numThreads;++i){
        futures.push_back(std::async(std::launch::async, DirServer(), 
                                     std::ref(dirQueue),
                                     std::ref(fileQueue)));
    }
    futures.push_back(std::async(std::launch::async, PrintServer(), 
                                                     std::ref(fileQueue)));

    try{
        while(!futures.empty()){
            auto ftr = std::move(futures.back());
            futures.pop_back();
            ftr.wait();
        }
    }catch(...){
        std::cout << "Exception thrown: " << std::endl;
    }
}

int main(int argc, const char *argv[])
{
    if(argc != 2){
        std::cout << "Usage: ./dirLists <directory>" << std::endl;
        return 1; 
    }

    listTree(path(argv[1]));    
    return 0;
}
