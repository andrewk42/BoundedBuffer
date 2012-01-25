/*
 * uC++ Concurrent BoundedBuffer Implementation
 *
 * Written by Andrew Klamut
 * January, 2012
 */

#include "uc_boundedbuffer.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;


/*****************************************************************************
 * helper functions...
 ****************************************************************************/

void usage(char *argv[]) {
    cerr << "Usage: "
         << argv[0]
         << " [ Cons (> 0) [ Prods (> 0) [ Produce (> 0) [ BufferSize (> 0) [ Delay (> 0) ] ] ] ] ]"
         << endl;

    exit(EXIT_FAILURE);
}

// Stolen from Peter Buhr
bool convert(int &val, char *buffer) {      // convert C string to integer
    std::stringstream ss(buffer);           // connect stream and buffer
    ss >> dec >> val;                       // convert integer from buffer
    return ! ss.fail() &&                   // conversion successful ?
    // characters after conversion all blank ?
    string( buffer ).find_first_not_of( " ", ss.tellg() ) == string::npos;
} // convert


/*****************************************************************************
 * BoundedBuffer
 ****************************************************************************/

template<typename T> BoundedBuffer<T>::BoundedBuffer(const unsigned int size) {
    buffer = new T[size];

    capacity = size;
    length = in_idx = out_idx = 0;
}

template<typename T> BoundedBuffer<T>::~BoundedBuffer() {
    delete buffer;
}

template<typename T> void BoundedBuffer<T>::insert(T elem) {
#if defined(BUSY)
    /* Protect this section from being accessed by multiple producers at once.
     * Could have used a different mutex lock instead of sharing one with remove,
     * but want to ensure that a producer/consumer cannot modify this.length at
     * the same time. */
    mlk.acquire();

    while (length == capacity) {
        // Block this task/reopen critical section if full
        p_clk.wait(mlk);
    }

    // If here, back in locked critical section

    // Perform assertion
    assert(length <= capacity);

    // Add element
    buffer[in_idx++ % capacity] = elem;
    ++length; // Probably atomic on your platform, but I assume it isn't

    // Tell the next consumer that there's an element, if someone is waiting
    c_clk.signal();

    // Leave critical section
    mlk.release();
#elif defined(NOBUSY)

#endif
}

template<typename T> T BoundedBuffer<T>::remove() {
#if defined(BUSY)
    /* Protect this section from being accessed by multiple consumers at once.
     * Could have used a different mutex lock instead of sharing one with insert,
     * but want to ensure that a producer/consumer cannot modify this.length at
     * the same time. */
    mlk.acquire();

    while (length == 0) {
        // Block this task/reopen critical section if empty
        c_clk.wait(mlk);
    }

    // If here, back in locked critical section

    // Perform assertion
    assert(length > 0);

    // Remove element
    T elem = buffer[out_idx++ % capacity];
    --length; // Probably atomic on your platform, but I assume it isn't

    // Tell the next producer that there's room in the buffer, if someone is waiting
    p_clk.signal();

    // Leave critical section
    mlk.release();

    return elem;
#elif defined(NOBUSY)

#endif
}


/*****************************************************************************
 * Task Producer
 ****************************************************************************/

Producer::Producer(BoundedBuffer<int> &buffer, Printer &p, const int Produce, const int Delay) : buffer(buffer), p(p) {
    num_items = Produce;
    delay_bound = Delay;
    id = count++;
}

void Producer::main() {
    // Indicate starting
    p.print(Printer::Producer, id, 'S');

    // Produce specified amount of items
    for (unsigned int i = 1; i <= num_items; i++) {
        // Yield to simulate work
        yield(rand() % delay_bound);

        // Indicate before adding value
        p.print(Printer::Producer, id, 'B', i);

        // Add item
        buffer.insert(i);

        // Indicate after adding value
        p.print(Printer::Producer, id, 'A', i);
    }

    // Indicate finishing
    p.print(Printer::Producer, id, 'F');
}


/*****************************************************************************
 * Task Consumer
 ****************************************************************************/

Consumer::Consumer(BoundedBuffer<int> &buffer, Printer &p, const int Delay, const int Sentinel, int &sum) : buffer(buffer), p(p), sum(sum) {
    delay_bound = Delay;
    sentinel = Sentinel;
    id = count++;
}

void Consumer::main() {
    int val = 0;

    // Indicate starting
    p.print(Printer::Consumer, id, 'S');

    for (;;) {
        // Yield to simulate work
        yield(rand() % delay_bound);

        // Indicate before consumption
        p.print(Printer::Consumer, id, 'B');

        // Take next value
        val = buffer.remove();

        // Indicate after consumption, value consumed
        p.print(Printer::Consumer, id, 'A', val);

        // Check for exit signal
        if (val == sentinel) break;

        // Get exclusive access to shared sum variable and add to it
        sum_mlk.acquire();
        sum += val;
        sum_mlk.release();
    }

    // Indicate finishing
    p.print(Printer::Consumer, id, 'F');
}


/*****************************************************************************
 * Monitor Printer
 ****************************************************************************/

Printer::Printer(unsigned int numProducers, unsigned int numConsumers) {
    unsigned int i;

    // Initialize private members
    num_producers = numProducers;
    num_consumers = numConsumers;

    for (i = 0; i < numProducers + numConsumers; i++) {
        buffer.push_back(BufferSlot());
    }

    // Print table headers
    for (i = 0; i < numProducers; i++) cout << "Prod:" << i << '\t';
    for (i = 0; i < numConsumers; i++) cout << "Cons:" << i << '\t';
    cout << endl;

    // Print table header underlines
    for (i = 0; i < numProducers + numConsumers - 1; i++) cout << "*******\t";
    cout << "*******" << endl;
}

void Printer::print(Kind kind, unsigned int lid, char state) {
    print(kind, lid, state, -1);
}

void Printer::print(Kind kind, unsigned int lid, char state, int value) {
    switch (kind) {
        case Producer:
            realPrint(lid, state, value);
            return;
        case Consumer:
            realPrint(num_producers + lid, state, value);
            return;
        default:
            uAbort("Invalid call to Printer::print()");
    }
}

void Printer::realPrint(unsigned int id, char state, int value) {
    // Print a line if this call is about to overwrite nonprinted values
    if (buffer[id].state != '\0' && (state != buffer[id].state || value != buffer[id].arg)) {
        flush();
    }

    // Check if this is the finish state, special case
    if (state == 'F') {
        for (unsigned int i = 0; i < buffer.size(); i++) {
            // Print 'F' for the finished item, "..." for the rest
            if (i == id) cout << 'F';
            else cout << "...";

            if (i != buffer.size()-1) cout << '\t';
        }

        cout << endl;

        clearSlot(id);

        return;
    }

    // Normal behaviour
    buffer[id].state = state;

    if (value != -1) buffer[id].arg = value;
}

void Printer::clearSlot(int id) {
    buffer[id].state = '\0';
    buffer[id].arg = -1;
}

void Printer::flush() {
    // Print something for every item in the buffer
    for (unsigned int i = 0; i < buffer.size(); i++) {
        // Skip this item if it is empty
        if (buffer[i].state == '\0') {
            cout << '\t';
            continue;
        }

        cout << buffer[i].state;

        // Print non-empty arguments
        if (buffer[i].arg != -1) cout << ':' << buffer[i].arg;

        // Print the tab if not the last column
        if (i != buffer.size()-1) cout << '\t';

        // Reset this item now that we've printed
        clearSlot(i);
    }

    cout << endl;
}


/*****************************************************************************
 * uMain
 ****************************************************************************/

void uMain::main() {
    int num_cons = 5, num_prods = 3, max_prod = 10, buffer_size = 10, delay_bound = 0;

#ifdef __U_MULTI__
    uProcessor p[3] __attribute__(( unused )); // create 3 kernel thread for a total of 4
#endif

    // Parse command line arguments
    switch (argc) {
      case 6:
        // Convert to int and quit if malformed
        if (!convert(delay_bound, argv[5])) usage(argv);

        // Verify argument semantics
        if (delay_bound < 1) usage(argv);

        // fall through
      case 5:
        if (!convert(buffer_size, argv[4])) usage(argv);

        if (buffer_size < 1) usage(argv);

      case 4:
        if (!convert(max_prod, argv[3])) usage(argv);

        if (max_prod < 1) usage(argv);

      case 3:
        if (!convert(num_prods, argv[2])) usage(argv);

        if (num_prods < 1) usage(argv);

      case 2:
        if (!convert(num_cons, argv[1])) usage(argv);

        if (num_cons < 1) usage(argv);

      case 1:
        // If no arguments just use initialized values
        break;
      default:
	    usage(argv);
    }

    // If Delay was not specified compute it now
    if (delay_bound == 0) delay_bound = num_cons + num_prods;

    // for testing
    cout << "Cons=" << num_cons << ", Prods=" << num_prods
         << ", Produce=" << max_prod << ", BufferSize=" << buffer_size
         << ", Delay=" << delay_bound << endl;

    // Begin
    int sum = 0;
    vector<Producer*> p_list;
    vector<Consumer*> c_list;

    // Create the Printer
    Printer *p = new Printer(num_prods, num_cons);

    // Create the single buffer
    BoundedBuffer<int> *b = new BoundedBuffer<int>(buffer_size);

    // Create specified amount of producers
    for (int i = 0; i < num_prods; i++) {
        p_list.push_back(new Producer(*b, *p, max_prod, delay_bound));
    }

    // Create specified amount of consumers
    for (int i = 0; i < num_cons; i++) {
        c_list.push_back(new Consumer(*b, *p, delay_bound, -1, sum));
    }

    // Wait for Producers to finish
    for (vector<Producer*>::iterator it = p_list.begin(); it != p_list.end(); it++) {
        delete *it;
    }

    // Send a sentinel value for every consumer that should finish
    for (vector<Consumer*>::iterator it = c_list.begin(); it != c_list.end(); it++) {
        b->insert(-1);
    }

    // Wait for Consumers to finish
    for (vector<Consumer*>::iterator it = c_list.begin(); it != c_list.end(); it++) {
        delete *it;
    }

    int expected = (1 + max_prod)*(((double)max_prod)/2)*num_prods;

    cout << "expected: " << expected << endl;
    cout << "total: " << sum << endl;

    // Cleanup
    delete b;
    delete p;
}
