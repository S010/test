#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <algorithm>
#include <functional>
#include <fstream>

// An uni-directional queue from a Producer to a Consumer.
// push() is used only by a Producer.
// pop() is used only by a Consumer.
template<typename T>
class Q {
    public:
        void push(const T& val) {
            _q_mutex.lock();
            _q.push(val);
            _q_grew.notify_one();
            _q_mutex.unlock();
        }

        T pop() {
            std::unique_lock<std::mutex> lock(_q_mutex);
            if (_q.size() == 0)
                _q_grew.wait(lock);
            T val(_q.front());
            _q.pop();
            lock.unlock();
            return val;
        }

    private:
        std::queue<T>           _q;
        std::mutex              _q_mutex;
        std::condition_variable _q_grew;
};

template<typename T>
class Consumer {
    public:
        Consumer(Q<T>& q):
            _q(q)
        {
        }

        void run() {
            std::ofstream out("out.txt");
            for ( ;; ) {
                std::string line(_q.pop());
                if (line == "quit")
                    return;
                out << line << std::endl;
            }
        }

    private:
        Q<T>& _q;
};

template<typename T>
class Producer {
    public:
        Producer():
            _consumer(_q)
        {
        }

        void run() {
            std::function<void(Consumer<T>)> _consumer_run = &Consumer<T>::run;
            std::thread consumer_thread(_consumer_run, _consumer);
            std::string line;
            while (getline(std::cin, line)) {
                _q.push(line);
                if (line == "quit") {
                    consumer_thread.join();
                    return;
                }
            }
        }

    private:
        Q<std::string> _q;
        Consumer<T> _consumer;
};

int
main(int argc, char** argv)
{
    Producer<std::string> producer;

    producer.run();

    return 0;
}
