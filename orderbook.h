#ifndef ORDERBOOK_ORDERBOOK_H
#define ORDERBOOK_ORDERBOOK_H

#include <string>
#include <list>
#include <map>

typedef enum OrderSide {
    OrderSide_Buy = 'B',
    OrderSide_Sell = 'S',
} Side;

typedef enum OrderStatus {
    OrderStatus_New = '0',
    OrderStatus_PartiallyFilled = '1',
    OrderStatus_Filled = '2',
    OrderStatus_Canceled = '4',
    OrderStatus_Rejected = '8',
} OrderStatus;

struct Order {
    int orderId;
    OrderSide side;
    OrderStatus status;
    int orderQty;
    int cumQty;
    double orderPx;

    bool IsActive() {
        return status == OrderStatus_New || status == OrderStatus_PartiallyFilled;
    }

    std::string GetStatusString();
};

class OrderBook {

public:
    OrderBook() {}
    ~OrderBook();

    void NewOrder(int orderId, OrderSide side, int orderQty, double orderPx);
    void AmendOrder(int orderId, int orderQty);
    void CancelOrder(int orderId);

    Order *GetOrder(int orderId) {
        auto it = orders.find(orderId);
        return it == orders.end()? NULL: it->second;
    }

    //this function is not-optimized, its for Test assignment only
    //it is not on critical path of real-life order book
    //in real-life all we need is the top of buyPriceOrders & sellPriceOrders
    std::tuple<double, int, int> GetLevel(OrderSide side, int level);

    void ProcessMessage(const std::string &message, std::ostream &output);
    void Print(std::ostream &stream);

private:

    static bool ValidatePx(double px);
    int GetPosition(Order *order);

    void Match(Order *theOrder);
    void Deactivate(Order *order);

    void AddToPx(Order *order, double px);
    void AmendOnPx(Order *order, double px);
    void RemoveFromPx(Order *order, double px);

    std::map<int, Order *> orders;                                               //id -> orders map (all orders)
    std::map<double, std::list<Order *>, std::greater<double >> buyPriceOrders;  //price -> alive buy-side orders map
    std::map<double, std::list<Order *>> sellPriceOrders;                        //price -> alive sell-side orders map

};

#endif //ORDERBOOK_ORDERBOOK_H
