#include <unistd.h>
#include <csignal>
#include "fix_engine.h"
#include "exchange.h"
#include "msg_massquote.h"

class MyServer : public Acceptor {
    Exchange exchange;
public:
    MyServer() : Acceptor(9000,SessionConfig("SERVER","*")){};
    void onMessage(Session& session,const FixMessage& msg) override {
        if(msg.msgType()==MassQuote::msgType) {
            exchange.quote(session.id(),msg.getString(55),msg.getFixed(132),msg.getInt(134),msg.getFixed(133),msg.getInt(135),msg.getString(302));
            FixBuilder fix(256);
            MassQuoteAck::build(fix,msg.getString(117),0);
            session.sendMessage(MassQuoteAck::msgType,fix);
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