/*
 * CS343 A3 BoundedBuffer (Q1) Implementation
 *
 * Written by Andrew Klamut
 * Born October 20, 2011
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

}

template<typename T> BoundedBuffer<T>::~BoundedBuffer() {
}

template<typename T> void BoundedBuffer<T>::insert(T elem) {
}

template<typename T> T BoundedBuffer<T>::remove() {
}


/*****************************************************************************
 * Task Producer
 ****************************************************************************/

Producer::Producer(BoundedBuffer<int> &buffer, Printer &p, const int Produce, const int Delay) {
}

void Producer::main() {
}


/*****************************************************************************
 * Task Consumer
 ****************************************************************************/

Consumer::Consumer(BoundedBuffer<int> &buffer, Printer &p, const int Delay, const int Sentinel, int &sum) {
}

void Consumer::main() {
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
    /*for (int i = 0; i < num_prods; i++) {
        p_list.push_back(new Producer(*b, max_prod, delay_bound));
    }

    // Create specified amount of consumers
    for (int i = 0; i < num_cons; i++) {
        c_list.push_back(new Consumer(*b, delay_bound, -1, sum));
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

    cout << "total: " << sum << endl;*/

    // Cleanup
    delete b;
    delete p;
}
