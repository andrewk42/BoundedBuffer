/*
 * uC++ Concurrent BoundedBuffer Interface
 *
 * Written by Andrew Klamut
 * January, 2012
 */

#include <uC++.h>
#include <vector>

template<typename T> class BoundedBuffer {
    T *buffer;
    unsigned int capacity, length, in_idx, out_idx;
    uOwnerLock mlk; // mutex lock
    uCondLock p_clk, c_clk; // condition locks
#ifdef NOBUSY
    bool barge_flag;
    uCondLock b_clk; // barge condition lock
#endif

  public:
    BoundedBuffer(const unsigned int size = 10);
    ~BoundedBuffer();
    void insert(T, int);
    T remove(int);
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
    BoundedBuffer<int> &buffer;
    Printer &p;
    unsigned int id, num_items, delay_bound;
    static unsigned int count;

    void main();
  public:
    Producer(BoundedBuffer<int> &buffer, Printer &p, const int Produce, const int Delay);
};

_Task Consumer {
    BoundedBuffer<int> &buffer;
    Printer &p;
    uOwnerLock sum_mlk;
    int id, delay_bound, sentinel, &sum;
    static unsigned int count;

    void main();
  public:
    Consumer(BoundedBuffer<int> &buffer, Printer &p, const int Delay, const int Sentinel, int &sum);
};

unsigned int Producer::count = 0;
unsigned int Consumer::count = 0;
