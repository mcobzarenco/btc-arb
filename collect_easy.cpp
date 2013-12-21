#include "easywsclient.hpp"

#include <assert.h>
#include <string>
#include <memory>
#include <iostream>


using namespace std;

int main()
{
    using easywsclient::WebSocket;

    std::unique_ptr<WebSocket> ws(WebSocket::from_url("ws://websocket.mtgox.com/mtgox"));
    assert(ws);
    // ws->send("goodbye");
    // ws->send("hello");
    while (ws->getReadyState() != WebSocket::CLOSED) {
        cout << "reading msg.." << endl;
        WebSocket::pointer wsp = &*ws; // <-- because a unique_ptr cannot be copied into a lambda
        ws->poll();
        ws->dispatch([wsp](const std::string & message) {
                cout << message << endl;
                // if (message == "world") { wsp->close(); }
            });
    }
    return 0;
}
