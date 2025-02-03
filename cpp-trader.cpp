#include <unistd.h>
#include <cfloat>
#include <csignal>
#include "fix_engine.h"
#include "exchange.h"
#include "msg_massquote.h"
#include "msg_orders.h"

class MyServer : public Acceptor<> , public ExchangeListener {
    Exchange exchange;
public:
    MyServer() : Acceptor(9000,DefaultSessionConfig("SERVER","*"),std::max(int(std::thread::hardware_concurrency()/2),1)), exchange(*this){};
    void onMessage(Session<>& session,const FixMessage& msg) override {
        auto msgType = msg.msgType();
        if(msgType==MassQuote::msgType) {
            exchange.quote(session.id(),msg.getString(Tag::SYMBOL),msg.getFixed(132),msg.getInt(134),msg.getFixed(133),msg.getInt(135),msg.getString(302));
            FixBuilder fix(256);
            MassQuoteAck::build(fix,msg.getString(117),0);
            session.sendMessage(MassQuoteAck::msgType,fix);
        } else if(msgType==NewOrderSingle::msgType) {
            auto side = msg.getInt(Tag::SIDE)==1 ? Order::BUY : Order::SELL;
            auto orderType = msg.getInt(Tag::ORD_TYPE)==1 ? OrderType::Market : OrderType::Limit;
            auto price = orderType==OrderType::Market ? side == Order::BUY ? DBL_MAX : -DBL_MAX : msg.getFixed(Tag::PRICE);
            std::cout << "new order " << msg.getString(Tag::CLORDID) << " " << side << " " << orderType << " "<< msg.getString(Tag::SYMBOL) << " " << price << " " << msg.getInt(Tag::ORDER_QTY) << "\n";
            if (side == Order::BUY) {
                exchange.buy(session.id(),msg.getString(Tag::SYMBOL),price,msg.getInt(Tag::ORDER_QTY),msg.getString(Tag::CLORDID));
            } else {
                exchange.sell(session.id(),msg.getString(Tag::SYMBOL),price,msg.getInt(Tag::ORDER_QTY),msg.getString(Tag::CLORDID));
            }
        } else if(msgType==OrderCancelRequest::msgType) {
            auto exchangeId = msg.getLong(37);
            std::cout << "request to cancel order, exchange id "<< exchangeId <<"\n";
            int status = 0;
            try {
                status = exchange.cancel(exchangeId,session.id());
            } catch (std::exception& e) {
                std::cerr << "error cancelling order " << e.what() << "\n";
                status = -1;
            }
            FixBuilder fix(256);
            if(status!=0) {
                std::cout << "order " << exchangeId << " not found\n";
                OrderCancelReject::build(fix,exchangeId,msg.getString(Tag::CLORDID), OrderStatus::Rejected);
                session.sendMessage(OrderCancelReject::msgType,fix);
            }
        }
    }
    
    /** callback when order status changes */
    virtual void onOrder(const Order& order) override {
        if(order.isQuote()) {
            return;
        }
        FixBuilder fix(256);
        auto execType = ExecType::Status;
        auto orderStatus = order.isCancelled() ? OrderStatus::Canceled : order.isFilled() ? OrderStatus::Filled : order.cumulativeQuantity()==0 ? OrderStatus::New : OrderStatus::PartiallyFilled;
        auto orderSide = order.side==Order::BUY ? OrderSide::Buy : OrderSide::Sell;
        ExecutionReport::build<7>(fix,order.orderId(), order.instrument, orderSide, 0, 0, order.cumulativeQuantity(), order.averagePrice(), order.remainingQuantity(),order.exchangeId,execType,0,orderStatus);
        try {
            // session may be disconnected
            sendMessage(order.sessionId(),ExecutionReport::msgType,fix);
        } catch (std::exception& e) {
            std::cerr << "error sending message " << e.what() << "\n";
        }
    };

    /** callback when trade occurs */
    virtual void onTrade(const Trade& trade) override {
        FixBuilder fix(256);
        for (auto order : {trade.aggressor,trade.opposite}) {
            auto execType = order.remainingQuantity()==0 ? ExecType::Filled : ExecType::PartiallyFilled;
            auto orderStatus = order.remainingQuantity()==0 ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
            auto orderSide = order.side==Order::BUY ? OrderSide::Buy : OrderSide::Sell;
            ExecutionReport::build<7>(fix,order.orderId(), order.instrument, orderSide, trade.price, trade.quantity, order.cumulativeQuantity(), order.averagePrice(), order.remainingQuantity(),order.exchangeId,execType,trade.execId,orderStatus);
            try {
                // session may be disconnected
                sendMessage(order.sessionId(),ExecutionReport::msgType,fix);
            } catch (std::exception& e) {
                std::cerr << "error sending message " << e.what() << "\n";
            }
        }
    };

    bool validateLogon(const FixMessage& msg) override {
        return true;
    }
    void onLoggedOn(const Session<>& session) override {
        std::cout << "logon " << session.id() << "\n";
        Acceptor::onLoggedOn(session);
    }
    void dumpBooks() {
        std::cout << "dumping books...\n";
        auto instruments = exchange.instruments();
        for(auto& instrument : instruments) {
            auto book = exchange.book(instrument);
            std::cout << "Book " << instrument << ":\n";
            std::cout << book;
        }
    }
    void onDisconnected(const Session<>& session) override {
        std::cout << "disconnected " << session.id() << "\n";
        for(auto order : exchange.orders()) {
            if(order->sessionId()==session.id()) {
                if(order->isActive()) {
                    exchange.cancel(order->exchangeId,session.id());
                }
            }
        }
        Acceptor::onDisconnected(session);
    }
};

static std::atomic<bool> displayBooks = false;

int main(int argc, char* argv[]) {
    MyServer server;
    auto thread = std::thread([&](){
        server.listen();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "send USR1 to " << getpid() << " to display books\n";
    
    struct sigaction sa;
    sa.sa_handler = [](int){ displayBooks = true; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);

    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(displayBooks) {
            displayBooks = false;
            server.dumpBooks();
        }
    }
}