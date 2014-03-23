///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2014 Tavendo GmbH
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef AUTOBAHN_HPP
#define AUTOBAHN_HPP

#include <stdexcept>
#include <istream>
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

#include <msgpack.hpp>


#define MSG_CODE_HELLO 1
#define MSG_CODE_WELCOME 2
#define MSG_CODE_ABORT 3
#define MSG_CODE_CHALLENGE 4
#define MSG_CODE_AUTHENTICATE 5
#define MSG_CODE_GOODBYE 6
#define MSG_CODE_HEARTBEAT 7
#define MSG_CODE_ERROR 8
#define MSG_CODE_PUBLISH 16
#define MSG_CODE_PUBLISHED 17
#define MSG_CODE_SUBSCRIBE 32
#define MSG_CODE_SUBSCRIBED 33
#define MSG_CODE_UNSUBSCRIBE 34
#define MSG_CODE_UNSUBSCRIBED 35
#define MSG_CODE_EVENT 36
#define MSG_CODE_CALL 48
#define MSG_CODE_CANCEL 49
#define MSG_CODE_RESULT 50
#define MSG_CODE_REGISTER 64
#define MSG_CODE_REGISTERED 65
#define MSG_CODE_UNREGISTER 66
#define MSG_CODE_UNREGISTERED 67
#define MSG_CODE_INVOCATION 68
#define MSG_CODE_INTERRUPT 69
#define MSG_CODE_YIELD 70


class ProtocolError : public std::runtime_error {
   public:
      ProtocolError(const std::string& msg);
};

class Unimplemented : public std::runtime_error {
   public:
      Unimplemented(const std::string& msg, int type_code = 0);
      virtual const char* what() const throw();
   private:
      const int m_type_code;
};


class Value {
   public:
      template<typename T>
      void add(const T& arg) {
         _args.push_back(msgpack::object(arg));
      }

      std::vector<msgpack::object> _args;

};


#include <boost/any.hpp>

using boost::any_cast;
typedef std::map<std::string, boost::any> anymap;
typedef std::vector<boost::any> anyvec;


class WampSession {
   public:
      WampSession(std::istream& in, std::ostream& out);

      void send_hello(const std::string& realm);

      void process();

      typedef std::vector<msgpack::object> args;

      void publish(const std::string& topic, args& args);

      void publish(const std::string& topic, Value& args);

      void publish(const std::string& topic, anyvec& args);

      void pack_any(const boost::any& value);

   private:

      void send();
      bool receive();

      std::istream& m_in;
      std::ostream& m_out;

      msgpack::sbuffer m_buffer;
      msgpack::packer<msgpack::sbuffer> m_packer;
      msgpack::unpacker m_unpacker;

      uint64_t m_session_id;
};

#endif // AUTOBAHN_HPP
