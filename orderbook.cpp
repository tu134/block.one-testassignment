
#include <vector>

#include <boost/algorithm/string.hpp>

#include "orderbook.h"

using namespace std;

OrderBook::~OrderBook() {
    for(auto it: orders) {
        delete it.second;
    }
    orders.clear();
}

void OrderBook::NewOrder(int orderId, OrderSide side, int orderQty, double orderPx){

    if(orders.find(orderId) != orders.end()) {
        //https://www.onixs.biz/fix-dictionary/4.2/app_d22.html
        //for this Test Assignment we do nothing ;)
        return;
    }

    if(!ValidatePx(orderPx)) {
        return;
    }

    Order *order = new Order();

    order->orderId = orderId;
    order->side = side;
    order->orderQty = orderQty;
    order->cumQty = 0;
    order->orderPx = orderPx;
    order->status = OrderStatus_New;
    orders[orderId] = order;

    AddToPx(order, orderPx);
    Match(order);
}

void OrderBook::AmendOrder(int orderId, int orderQty){
    if(orders.find(orderId) == orders.end()) {
        //amend non-existing order
        return;
    }

    Order *order = orders[orderId];
    if(!order->IsActive()) {
        //assumption: we reject amends for non-active orders
        return;
    }

    if(orderQty <= order->cumQty) { //got fully filled -> removing
        order->orderQty = orderQty;
        order->status = OrderStatus_Filled;
        Deactivate(order);
        return;
    }

    if(order->orderQty < orderQty) { //amend up -> lost queue
        AmendOnPx(order, order->orderPx);
    }
    order->orderQty = orderQty;

    //TODO: for more aggressive price amends we will need Match
    // Match(order);
}

void OrderBook::CancelOrder(int orderId) {
    if(orders.find(orderId) == orders.end()) {
        //cancel non-existing order
        return;
    }

    Order *order = orders[orderId];
    if(!order->IsActive()) {
        //assumption: we reject cancel for non-active orders
        return;
    }

    order->status = OrderStatus_Canceled;
    Deactivate(order);
}

void OrderBook::Match(Order *theOrder) {

    int theLeavesQty = theOrder->orderQty - theOrder->cumQty;
    std::vector<Order *> completed;

    if(theOrder->side == OrderSide::OrderSide_Buy) {
        for(auto &iter : sellPriceOrders){
            auto px = iter.first;
            auto &orders = iter.second;
            if(px > theOrder->orderPx) break;

            for(auto order: orders) {
                if(!theLeavesQty) break;
                int leavesQty = order->orderQty - order->cumQty;
                if(theLeavesQty >= leavesQty) {
                    theLeavesQty -= leavesQty;
                    order->cumQty = order->orderQty;
                    order->status = OrderStatus_Filled;
                    completed.push_back(order);
                } else {
                    order->cumQty += theLeavesQty;
                    order->status = OrderStatus_PartiallyFilled;
                    theLeavesQty = 0;
                }
            }
        }
    } else {
        for(auto &iter : buyPriceOrders){
            auto px = iter.first;
            auto &orders = iter.second;
            if(px < theOrder->orderPx) break;

            for(auto order: orders) {
                if(!theLeavesQty) break;
                int leavesQty = order->orderQty - order->cumQty;
                if(theLeavesQty >= leavesQty) {
                    theLeavesQty -= leavesQty;
                    order->cumQty = order->orderQty;
                    order->status = OrderStatus_Filled;
                    completed.push_back(order);
                } else {
                    order->cumQty += theLeavesQty;
                    order->status = OrderStatus_PartiallyFilled;
                    theLeavesQty = 0;
                }
            }
        }
    }

    theOrder->cumQty = theOrder->orderQty - theLeavesQty;
    if(theOrder->cumQty == theOrder->orderQty) {
        theOrder->status = OrderStatus_Filled;
        completed.push_back(theOrder);
    } else if(theOrder->cumQty > 0) {
        theOrder->status = OrderStatus_PartiallyFilled;
    }
    for(auto order: completed) {
        Deactivate(order);
    }
}

void OrderBook::Deactivate(Order *order) {
    RemoveFromPx(order, order->orderPx);
}

void OrderBook::AddToPx(Order *order, double px){
    //pre-condition: order must not be on this px
    if(order->side == OrderSide::OrderSide_Buy) {
        if(buyPriceOrders.find(px) == buyPriceOrders.end())
            buyPriceOrders[px] = std::list<Order *>();
        buyPriceOrders[px].push_back(order);
    } else {
        if (sellPriceOrders.find(order->orderPx) == sellPriceOrders.end())
            sellPriceOrders[order->orderPx] = std::list<Order *>();
        sellPriceOrders[order->orderPx].push_back(order);
    }
}

void OrderBook::AmendOnPx(Order *order, double px){
    //pre-condition: order must be on this px
    if(order->side == OrderSide::OrderSide_Buy) {
        auto &priceOrders = buyPriceOrders[px];
        priceOrders.remove(order);
        priceOrders.push_back(order);
    } else {
        auto &priceOrders = sellPriceOrders[px];
        priceOrders.remove(order);
        priceOrders.push_back(order);
    }
}

void OrderBook::RemoveFromPx(Order *order, double px) {
    //pre-condition: order must be on this px already
    if(order->side == OrderSide::OrderSide_Buy) {
        auto &priceOrders = buyPriceOrders[px];
        priceOrders.remove(order);
        if(!priceOrders.size())
            buyPriceOrders.erase(px);
    }
    else {
        auto &priceOrders = sellPriceOrders[px];
        priceOrders.remove(order);
        if(!priceOrders.size())
            sellPriceOrders.erase(px);
    }
}

std::tuple<double, int, int> OrderBook::GetLevel(OrderSide side, int level) {
    int i = 0;
    list < Order * > orders;
    double px = 0;
    if (side == OrderSide::OrderSide_Sell) {
        auto it = sellPriceOrders.begin();
        for (; i < level && it != sellPriceOrders.end(); ++i, ++it) {}
        if (i == level && it != sellPriceOrders.end()) {
            px = it->first;
            orders = it->second;
        }
    } else {
        auto it = buyPriceOrders.begin();
        for (; i < level && it != buyPriceOrders.end(); ++i, ++it) {}
        if (i == level && it != buyPriceOrders.end()) {
            px = it->first;
            orders = it->second;
        }
    }
    if(orders.size() == 0) return tuple<double, int, int>(0, 0, 0);

    int qty = 0;
    int count = 0;
    for(Order * order: orders) {
        qty += order->orderQty - order->cumQty;
        ++count;
    }
    return tuple<double, int, int>(px, qty, count);
}

int OrderBook::GetPosition(Order *theOrder){

    if(!theOrder->IsActive()) return -1;

    list<Order *> orders;
    if(theOrder->side == OrderSide::OrderSide_Buy) {
        orders = buyPriceOrders[theOrder->orderPx];
    } else {
        orders = sellPriceOrders[theOrder->orderPx];
    }
    int pos = 0;
    for(Order *order: orders) {
        if(order == theOrder) break;
        ++pos;
    }
    return pos == orders.size()? -1: pos;
}


void OrderBook::Print(std::ostream &stream) {
    stream << "Bid/Ask: " <<endl;
    int count = 0;
    for(auto &it: buyPriceOrders){
        if(++count > 5) break;
        double px = it.first;
        int qty = 0;
        for(Order *order: it.second){
            qty += order->orderQty - order->cumQty;
        }
        stream << "Bid " << count << ": px=" << px << ", qty=" << qty <<endl;
    }
    count = 0;
    for(auto &it: sellPriceOrders){
        if(++count > 5) break;
        double px = it.first;
        int qty = 0;
        for(Order *order: it.second){
            qty += order->orderQty - order->cumQty;
        }
        stream << "Ask " << count << ": px=" << px << ", qty=" << qty <<endl;
    }

    stream << "Orders: " <<endl;
    for(auto it: orders) {
        Order *order = it.second;
        stream << "orderId=" << order->orderId
                << ", px=" << order->orderPx
                << ", qty=" << order->orderQty
                << ", cumQty=" << order->cumQty
                << ", status=" << order->GetStatusString()
                <<endl;
    }

}

class InvalidException : public std::exception {
};

void OrderBook::ProcessMessage(const std::string &message, std::ostream &output ) {
    try {

        std::deque<std::string> tokens;
        boost::split(tokens, message, boost::is_any_of(" "));
        if (tokens.size() < 1) throw InvalidException();

        string messageType = tokens[0];
        if(messageType == "order") {
            //order 1001 buy 100 12.30
            if (tokens.size() < 5) throw InvalidException();
            int orderId = std::stoi(tokens[1]);
            std::string strSide = tokens[2];
            OrderSide side;
            if(strSide == "buy") {
                side = OrderSide::OrderSide_Buy;
            } else if(strSide == "sell") {
                side = OrderSide::OrderSide_Sell;
            } else {
                throw InvalidException();
            }
            int orderQty = std::stoi(tokens[3]);
            double orderPx = std::stod(tokens[4]);

            if (orderId < 0 || orderQty <= 0 || orderPx <= 0)
                throw InvalidException();

            NewOrder(orderId, side, orderQty, orderPx);
        } else if(messageType == "amend") {
            //M,100007,S,4,1025
            //amend 1004 600
            if (tokens.size() < 3) throw InvalidException();
            int orderId = std::stoi(tokens[1]);
            int orderQty = std::stoi(tokens[2]);

            if (orderId < 0 || orderQty <= 0) throw InvalidException();

            AmendOrder(orderId, orderQty);
        } else if(messageType == "cancel") {
            //cancel 1003
            if (tokens.size() < 2) throw InvalidException();
            int orderId = std::stoi(tokens[1]);
            if (orderId < 0) throw InvalidException();

            CancelOrder(orderId);
        } else if(messageType == "q") {
            if (tokens.size() < 2) throw InvalidException();
            string subType = tokens[1];
            if(subType == "level") {
                if (tokens.size() < 4) throw InvalidException();
                string subsubType = tokens[2];
                int level = std::stoi(tokens[3]);
                if(subsubType == "ask") {
                    auto info = GetLevel(OrderSide::OrderSide_Sell, level);
                    output << "ask, " << level << ", " << std::get<0>(info) << ", " << std::get<1>(info) << ", " << std::get<2>(info) << endl;
                } else if(subsubType == "bid") {
                    auto info = GetLevel(OrderSide::OrderSide_Buy, level);
                    output << "bid, " << level << ", " << std::get<0>(info) << ", " << std::get<1>(info) << ", " << std::get<2>(info) << endl;
                } else {
                    throw InvalidException();
                }
            } else if(subType == "order") {
                if (tokens.size() < 3) throw InvalidException();
                int orderId = std::stoi(tokens[2]);
                Order *order = GetOrder(orderId);
                if(order != nullptr) {
                    output << "order, " << orderId << ", " << order->GetStatusString() << ", " <<
                    order->orderQty - order->cumQty << ", " << GetPosition(order) << endl;
                } else {
                    output << "order, " << orderId << " " << "Not found" << endl;
                }
            } else {
                throw InvalidException();
            }
        } else {
            throw InvalidException();
        }

    }

    catch (...) { //InvalidException, std::invalid_argument, std::out_of_range
        output << "Got Invalid Message: " << message << endl;
    }
}

 bool OrderBook::ValidatePx(double px) {
    //TODO: add logic for validation of tick size
    // for this Test Assignment we always return true
    // the exact logic depends always exchange
    // for example for SGX Stocks, REITs, business trusts, company warrants
    // Price Below 0.20 -> 0.001
    // Price 0.20 – 1.995 -> 0.005
    // Price 2.00 and above -> 0.01

    return true;
}

std::string Order::GetStatusString() {
    switch (status) {
        case OrderStatus_New:
            return "New";
        case OrderStatus_PartiallyFilled:
            return "PartiallyFilled";
        case OrderStatus_Filled:
            return "Filled";
        case OrderStatus_Canceled:
            return "Canceled";
        case OrderStatus_Rejected:
            return "Rejected";
        default:
            return "Unknown";
    }
}

