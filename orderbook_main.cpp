
#include <string>
#include <iostream>

#include "orderbook.h"

using namespace std;

int main() {
    OrderBook mgr;
    string line;

    while(getline(cin, line)) {
        mgr.ProcessMessage(line, cout);
    }
}

