#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <leveldb/db.h>

#include <iostream>
#include <memory>
#include <cstdlib>


typedef websocketpp::client<websocketpp::config::asio_client> ws_client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using namespace std;

void on_message(ws_client* c, leveldb::DB* db,
                websocketpp::connection_hdl hdl, message_ptr msg) {
  static int count = 0;
  ws_client::connection_ptr con = c->get_con_from_hdl(hdl);

  cout << "Received: " << msg->get_payload() << endl;

  leveldb::WriteOptions opts;
  stringstream key;
  key << (count++);
  db->Put(opts, key.str(), msg->get_payload());
  // if (con->get_resource() == "/getCaseCount") {
  //   std::cout << "Detected " << msg->get_payload() << " test cases."
  //             << std::endl;
  //   case_count = atoi(msg->get_payload().c_str());
  // } else {
  //   c->send(hdl, msg->get_payload(), msg->get_opcode());
  // }
}

void on_message2(ws_client* c, leveldb::DB* db,
                websocketpp::connection_hdl hdl, message_ptr msg) {
    static int count = 0;
    ws_client::connection_ptr con = c->get_con_from_hdl(hdl);

    cout << "RReceived: " << msg->get_payload() << endl;

    // leveldb::WriteOptions opts;
    // stringstream key;
    // key << (count++);
    // db->Put(opts, key.str(), msg->get_payload());
    // if (con->get_resource() == "/getCaseCount") {
    //   std::cout << "Detected " << msg->get_payload() << " test cases."
    //             << std::endl;
    //   case_count = atoi(msg->get_payload().c_str());
    // } else {
    //   c->send(hdl, msg->get_payload(), msg->get_opcode());
    // }
}

int main(int argc, char* argv[]) {
  ws_client client;
  const string uri = "ws://websocket.mtgox.com";
  const string ws_namespace = "/mtgox";

  try {
    // We expect there to be a lot of errors, so suppress them
    // c.clear_access_channels(websocketpp::log::alevel::all);
    // c.clear_error_channels(websocketpp::log::elevel::all);
    unique_ptr<leveldb::DB> db;
    {
      leveldb::DB *ptr_db{nullptr};
      leveldb::Options options;
      options.create_if_missing = true;
      leveldb::Status status = leveldb::DB::Open(options, "mdata.db", &ptr_db);
      db.reset(ptr_db);
    }
    if (!db) {
      cerr << "Could not open db. Exiting.." << endl;
      exit(1);
    }

    // Initialize ASIO
    client.init_asio();

    // Register our handlers
    client.set_message_handler(bind(&on_message,&client, db.get(), ::_1, ::_2));
    client.set_message_handler(bind(&on_message2,&client, db.get(), ::_1, ::_2));

    websocketpp::lib::error_code ec;
    ws_client::connection_ptr conn = client.get_connection(uri + ws_namespace, ec);
    conn->replace_header("Origin", uri);
    client.connect(conn);

    std::cout << "error: " << ec << std::endl;

    // Start the ASIO io_service run loop
    client.run();
  } catch (const std::exception & e) {
    std::cout << e.what() << std::endl;
  } catch (websocketpp::lib::error_code e) {
    std::cout << e.message() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }
}
