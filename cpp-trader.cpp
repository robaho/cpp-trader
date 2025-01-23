#include "fix_engine.h"
#include "exchange.h"
#include "msg_massquote.h"

class MyServer : public Acceptor {
    Exchange exchange;
public:
    MyServer() : Acceptor(9000,SessionConfig("SERVER","*")){};
    void onMessage(Session& session,const FixMessage& msg) {
        if(msg.msgType()==MassQuote::msgType) {
            exchange.quote(session.id(),msg.getString(55),msg.getFixed(132),msg.getInt(134),msg.getFixed(133),msg.getInt(135),msg.getString(302));
            FixBuilder fix(256);
            MassQuoteAck::build(fix,msg.getString(117),0);
            session.sendMessage(MassQuoteAck::msgType,fix);
        }
    }
    bool validateLogon(const FixMessage& msg) {
        return true;
    }
};

int main(int argc, char* argv[]) {
    MyServer server;
    server.listen();
}