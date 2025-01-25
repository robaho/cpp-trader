#include <unistd.h>
#include <cfloat>
#include <csignal>
#include "fix_engine.h"
#include "exchange.h"
#include "msg_massquote.h"
#include "msg_orders.h"

class MyServer : public Acceptor {
    Exchange exchange;
public:
    MyServer() : Acceptor(9000,SessionConfig("SERVER","*")){};
    void onMessage(Session& session,const FixMessage& msg) override {
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
            if (side == Order::BUY) {
                exchange.buy(session.id(),msg.getString(Tag::SYMBOL),price,msg.getInt(Tag::ORDER_QTY),msg.getString(Tag::CLORDID));
            } else {
                exchange.sell(session.id(),msg.getString(Tag::SYMBOL),price,msg.getInt(Tag::ORDER_QTY),msg.getString(Tag::CLORDID));
            }
        } else if(msgType==OrderCancelRequest::msgType) {
            auto exchangeId = msg.getLong(37);
            auto status = exchange.cancel(exchangeId);
            FixBuilder fix(256);
            auto orderStatus = status==0 ? OrderStatus::Canceled : OrderStatus::Rejected;
            OrderCancelReject::build(fix,exchangeId,msg.getString(Tag::CLORDID), orderStatus);
            session.sendMessage(OrderCancelReject::msgType,fix);
        }
    }
    bool validateLogon(const FixMessage& msg) override {
        return true;
    }
    void onLoggedOn(const Session& session) override {
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
    void onDisconnected(const Session& session) override {
        std::cout << "disconnected " << session.id() << "\n";
        for(auto order : exchange.orders()) {
            if(order->sessionId()==session.id()) {
                if(order->isActive()) {
                    exchange.cancel(order->exchangeId);
                }
            }
        }
        Acceptor::onDisconnected(session);
    }
};

static volatile bool displayBooks = false;

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