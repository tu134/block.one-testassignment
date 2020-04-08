#define BOOST_TEST_MODULE order_book_test
#include <boost/test/included/unit_test.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include "orderbook.h"

using namespace std;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_new ) {
    OrderBook mgr;
    int orderId = 100000;
    mgr.NewOrder(orderId, OrderSide::OrderSide_Sell, 1, 1075);
    Order * order1 = mgr.GetOrder(orderId);
    BOOST_CHECK_EQUAL(order1->orderQty, 1);
    BOOST_CHECK_EQUAL(order1->status, OrderStatus_New);
    BOOST_CHECK_EQUAL(order1->orderPx, 1075);
}

BOOST_AUTO_TEST_CASE( test_new_duplicated ) {
    OrderBook mgr;
    int orderId = 100000;
    mgr.NewOrder(orderId, OrderSide::OrderSide_Sell, 1, 1075);
    mgr.NewOrder(orderId, OrderSide::OrderSide_Sell, 2, 1075);
    Order * order = mgr.GetOrder(orderId);
    BOOST_CHECK_EQUAL(order->orderQty, 1);
}

BOOST_AUTO_TEST_CASE( test_amend ) {
    OrderBook mgr;
    int orderId = 100001;
    mgr.NewOrder(orderId, OrderSide::OrderSide_Sell, 1, 1075);
    mgr.AmendOrder(orderId, 2);
    Order * order = mgr.GetOrder(orderId);
    BOOST_CHECK_EQUAL(order->orderQty, 2);
}

BOOST_AUTO_TEST_CASE( test_amend_missing ) {
    OrderBook mgr;
    int orderId = 100001;
    mgr.AmendOrder(orderId, 1075);
    Order * order = mgr.GetOrder(orderId);
    BOOST_CHECK_EQUAL(order, nullptr);
}

BOOST_AUTO_TEST_CASE( test_cancel ) {
    OrderBook mgr;
    int orderId = 100001;
    mgr.NewOrder(orderId, OrderSide::OrderSide_Sell, 1, 1075);
    mgr.CancelOrder(orderId);
    Order * order = mgr.GetOrder(orderId);
    BOOST_CHECK_EQUAL(order->status, OrderStatus_Canceled);
}

BOOST_AUTO_TEST_CASE( test_executions ) {
    OrderBook mgr;
    mgr.NewOrder(10, OrderSide::OrderSide_Sell, 3, 1075);

    Order *order10 = mgr.GetOrder(10);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_New );

    mgr.NewOrder(11, OrderSide::OrderSide_Sell, 2, 1080);
    mgr.NewOrder(12, OrderSide::OrderSide_Buy, 1, 1000);
    mgr.NewOrder(16, OrderSide::OrderSide_Buy, 1, 2000);

    order10 = mgr.GetOrder(10);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_PartiallyFilled );
    BOOST_CHECK_EQUAL(order10->cumQty, 1 );

    mgr.NewOrder(30, OrderSide::OrderSide_Buy, 5, 2000);
    order10 = mgr.GetOrder(10);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_Filled );
    BOOST_CHECK_EQUAL(order10->cumQty, 3 );
}

BOOST_AUTO_TEST_CASE( test_executions_2 ) {
    OrderBook mgr;
    mgr.NewOrder(1001, OrderSide::OrderSide_Buy, 100, 12.3);
    mgr.NewOrder(1002, OrderSide::OrderSide_Sell, 100, 12.2);

    Order *order = mgr.GetOrder(1001);
    BOOST_CHECK_EQUAL(order->status, OrderStatus_Filled );
}


BOOST_AUTO_TEST_CASE( test_executions_cancel ) {
    OrderBook mgr;
    mgr.NewOrder(10, OrderSide::OrderSide_Sell, 3, 1075);
    mgr.CancelOrder(10);
    mgr.NewOrder(30, OrderSide::OrderSide_Buy, 5, 2000);
    Order *order10 = mgr.GetOrder(10);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_Canceled );
    BOOST_CHECK_EQUAL(order10->cumQty, 0 );
}


BOOST_AUTO_TEST_CASE( test_executions_multi_price_priority ) {
    OrderBook mgr;
    mgr.NewOrder(11, OrderSide::OrderSide_Sell, 2, 1080);
    mgr.NewOrder(10, OrderSide::OrderSide_Sell, 3, 1075);
    mgr.NewOrder(12, OrderSide::OrderSide_Buy, 1, 4000);
    Order *order10 = mgr.GetOrder(10);
    Order *order11 = mgr.GetOrder(11);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_PartiallyFilled );
    BOOST_CHECK_EQUAL(order10->cumQty, 1 );
    BOOST_CHECK_EQUAL(order11->status, OrderStatus_New );
    BOOST_CHECK_EQUAL(order11->cumQty, 0 );


}

BOOST_AUTO_TEST_CASE( test_executions_time_priority ) {
    OrderBook mgr;
    mgr.NewOrder(10, OrderSide::OrderSide_Sell, 2, 1080);
    mgr.NewOrder(11, OrderSide::OrderSide_Sell, 3, 1080);
    mgr.NewOrder(12, OrderSide::OrderSide_Buy, 1, 4000);
    Order *order10 = mgr.GetOrder(10);
    Order *order11 = mgr.GetOrder(11);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_PartiallyFilled );
    BOOST_CHECK_EQUAL(order10->cumQty, 1 );
    BOOST_CHECK_EQUAL(order11->status, OrderStatus_New );
    BOOST_CHECK_EQUAL(order11->cumQty, 0 );

}

BOOST_AUTO_TEST_CASE( test_executions_time_priority_amend_down ) {
    OrderBook mgr;
    mgr.NewOrder(10, OrderSide::OrderSide_Sell, 5, 1080);
    mgr.NewOrder(11, OrderSide::OrderSide_Sell, 3, 1080);
    mgr.AmendOrder(10, 2);
    mgr.NewOrder(12, OrderSide::OrderSide_Buy, 1, 4000);
    Order *order10 = mgr.GetOrder(10);
    Order *order11 = mgr.GetOrder(11);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_PartiallyFilled );
    BOOST_CHECK_EQUAL(order10->cumQty, 1 );
    BOOST_CHECK_EQUAL(order11->status, OrderStatus_New );
    BOOST_CHECK_EQUAL(order11->cumQty, 0 );
}

BOOST_AUTO_TEST_CASE( test_executions_time_priority_amend_up ) {
    OrderBook mgr;
    mgr.NewOrder(10, OrderSide::OrderSide_Sell, 2, 1080);
    mgr.NewOrder(11, OrderSide::OrderSide_Sell, 3, 1080);
    mgr.AmendOrder(10, 5);
    mgr.NewOrder(12, OrderSide::OrderSide_Buy, 1, 4000);
    Order *order10 = mgr.GetOrder(10);
    Order *order11 = mgr.GetOrder(11);
    BOOST_CHECK_EQUAL(order10->status, OrderStatus_New );
    BOOST_CHECK_EQUAL(order10->cumQty, 0 );
    BOOST_CHECK_EQUAL(order11->status, OrderStatus_PartiallyFilled );
    BOOST_CHECK_EQUAL(order11->cumQty, 1 );
}

BOOST_AUTO_TEST_CASE( test_level_buy ) {
    OrderBook mgr;
    mgr.NewOrder(1001, OrderSide::OrderSide_Buy, 100, 12.30);
    mgr.NewOrder(1003, OrderSide::OrderSide_Buy, 200, 12.40);
    mgr.NewOrder(1004, OrderSide::OrderSide_Buy, 300, 12.15);
    auto ret1 = mgr.GetLevel(OrderSide::OrderSide_Sell, 0);
    auto ret2 = mgr.GetLevel(OrderSide::OrderSide_Buy, 0);
    auto ret3 = mgr.GetLevel(OrderSide::OrderSide_Buy, 1);
    auto ret4 = mgr.GetLevel(OrderSide::OrderSide_Buy, 2);
    auto ret5 = mgr.GetLevel(OrderSide::OrderSide_Buy, 3);

    BOOST_CHECK_EQUAL(get<0>(ret1), 0 );
    BOOST_CHECK_EQUAL(get<1>(ret1), 0 );
    BOOST_CHECK_EQUAL(get<0>(ret2), 12.40 );
    BOOST_CHECK_EQUAL(get<1>(ret2), 200 );
    BOOST_CHECK_EQUAL(get<0>(ret3), 12.30 );
    BOOST_CHECK_EQUAL(get<1>(ret3), 100 );
    BOOST_CHECK_EQUAL(get<0>(ret4), 12.15 );
    BOOST_CHECK_EQUAL(get<1>(ret4), 300 );
    BOOST_CHECK_EQUAL(get<0>(ret5), 0 );
    BOOST_CHECK_EQUAL(get<1>(ret5), 0 );
}

BOOST_AUTO_TEST_CASE( test_level_sell ) {
    OrderBook mgr;
    mgr.NewOrder(1001, OrderSide::OrderSide_Sell, 100, 12.30);
    mgr.NewOrder(1003, OrderSide::OrderSide_Sell, 200, 12.40);
    mgr.NewOrder(1004, OrderSide::OrderSide_Sell, 300, 12.15);
    auto ret1 = mgr.GetLevel(OrderSide::OrderSide_Buy, 0);
    auto ret4 = mgr.GetLevel(OrderSide::OrderSide_Sell, 0);
    auto ret3 = mgr.GetLevel(OrderSide::OrderSide_Sell, 1);
    auto ret2 = mgr.GetLevel(OrderSide::OrderSide_Sell, 2);
    auto ret5 = mgr.GetLevel(OrderSide::OrderSide_Sell, 3);

    BOOST_CHECK_EQUAL(get<0>(ret1),0 );
    BOOST_CHECK_EQUAL(get<1>(ret1),0 );
    BOOST_CHECK_EQUAL(get<0>(ret2),12.40 );
    BOOST_CHECK_EQUAL(get<1>(ret2),200 );
    BOOST_CHECK_EQUAL(get<0>(ret3),12.30 );
    BOOST_CHECK_EQUAL(get<1>(ret3),100 );
    BOOST_CHECK_EQUAL(get<0>(ret4),12.15 );
    BOOST_CHECK_EQUAL(get<1>(ret4),300 );
    BOOST_CHECK_EQUAL(get<0>(ret5),0 );
    BOOST_CHECK_EQUAL(get<1>(ret5),0 );
}

BOOST_AUTO_TEST_CASE( test_processmessage ) {
    OrderBook mgr;
    stringstream s, s1, s2, s3, s4;
    mgr.ProcessMessage("order 1001 buy 100 12.3", s);
    mgr.ProcessMessage("order 1002 sell 100 12.2", s);
    mgr.ProcessMessage("order 1003 buy 200 12.4", s);
    mgr.ProcessMessage("order 1004 buy 300 12.15", s);
    mgr.ProcessMessage("cancel 1003", s);
    mgr.ProcessMessage("amend 1004 600", s);
    mgr.ProcessMessage("q level ask 0", s1);
    mgr.ProcessMessage("q level bid 0", s2);
    mgr.ProcessMessage("q order 1004", s3);
    mgr.ProcessMessage("q order 1001", s4);

    BOOST_CHECK_EQUAL(boost::trim_copy(s1.str()), "ask, 0, 0, 0, 0");
    BOOST_CHECK_EQUAL(boost::trim_copy(s2.str()), "bid, 0, 12.15, 600, 1");
    BOOST_CHECK_EQUAL(boost::trim_copy(s3.str()), "order, 1004, New, 600, 0");
    BOOST_CHECK_EQUAL(boost::trim_copy(s4.str()), "order, 1001, Filled, 0, -1");
}

