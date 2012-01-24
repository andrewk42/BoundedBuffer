/*
 * uC++ Concurrent BoundedBuffer Interface
 *
 * Written by Andrew Klamut
 * January, 2012
 */

#include <uC++.h>
#include <vector>

template<typename T> class BoundedBuffer {
  public:
    BoundedBuffer(const unsigned int size = 10);
    ~BoundedBuffer();
    void insert(T);
    T remove();
};

_Monitor Printer {
    struct BufferSlot {
        BufferSlot() : state('\0'), arg(-1) {}
        char state;
        int arg;
    };

    unsigned int num_producers, num_consumers;
    std::vector<BufferSlot> buffer;
    void realPrint(unsigned int, char, int);
    void clearSlot(int);
    void flush();

  public:
    enum Kind {Producer, Consumer};
    Printer(unsigned int numProducers, unsigned int numConsumers);
    void print(Kind kind, unsigned int lid, char state);
    void print(Kind kind, unsigned int lid, char state, int value);
};

_Task Producer {
    void main();
  public:
    Producer(BoundedBuffer<int> &buffer, Printer &p, const int Produce, const int Delay);
};

_Task Consumer {
    void main();
  public:
    Consumer(BoundedBuffer<int> &buffer, Printer &p, const int Delay, const int Sentinel, int &sum);
};
