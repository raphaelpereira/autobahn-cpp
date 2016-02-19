///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Tavendo GmbH
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include "parameters.hpp"

#include <autobahn/autobahn.hpp>
#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>

void add2(autobahn::wamp_invocation invocation)
{
    auto a = invocation->argument<uint64_t>(0);
    auto b = invocation->argument<uint64_t>(1);

    std::cerr << "Procedure com.examples.calculator.add2 invoked: " << a << ", " << b << std::endl;

    invocation->result(std::make_tuple(a + b));
}

void longop(autobahn::wamp_invocation invocation)
{
    auto a = invocation->argument<uint64_t>(0);
    std::cerr << "Procedure com.myapp.longop invoked: " << a << std::endl;
    int i = 0;
    for (; i < a; i++)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
        if (i < a)
        {
            invocation->result(std::make_tuple(i), true);
        }
    }
    invocation->result(std::make_tuple(i));
}

int main(int argc, char** argv)
{
    std::cerr << "Boost: " << BOOST_VERSION << std::endl;
    try {
        auto parameters = get_parameters(argc, argv);

        std::cerr << "Connecting to realm: " << parameters->realm() << std::endl;

        boost::asio::io_service io;
        bool debug = parameters->debug();

        auto transport = std::make_shared<autobahn::wamp_tcp_transport>(
                io, parameters->rawsocket_endpoint(), debug);

        auto session = std::make_shared<autobahn::wamp_session>(io, debug);

        transport->attach(std::static_pointer_cast<autobahn::wamp_transport_handler>(session));

        // Make sure the continuation futures we use do not run out of scope prematurely.
        // Since we are only using one thread here this can cause the io service to block
        // as a future generated by a continuation will block waiting for its promise to be
        // fulfilled when it goes out of scope. This would prevent the session from receiving
        // responses from the router.
        boost::future<void> connect_future;
        boost::future<void> start_future;
        boost::future<void> join_future;
        boost::future<void> provide_future;

        connect_future = transport->connect().then([&](boost::future<void> connected) {
            try {
                connected.get();
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                io.stop();
                return;
            }

            std::cerr << "transport connected" << std::endl;

            start_future = session->start().then([&](boost::future<void> started) {
                try {
                    started.get();
                } catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    io.stop();
                    return;
                }

                std::cerr << "session started" << std::endl;

                join_future = session->join(parameters->realm()).then([&](boost::future<uint64_t> joined) {
                    try {
                        std::cerr << "joined realm: " << joined.get() << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << e.what() << std::endl;
                        io.stop();
                        return;
                    }

                    provide_future = session->provide("com.examples.calculator.add2", &add2).then(
                        [&](boost::future<autobahn::wamp_registration> registration) {
                        try {
                            std::cerr << "registered procedure:" << registration.get().id() << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << e.what() << std::endl;
                            io.stop();
                            return;
                        }
                    });

                    provide_future = session->provide("com.myapp.longop", &longop).then(
                        [&](boost::future<autobahn::wamp_registration> registration) {
                        try {
                            std::cerr << "registered procedure:" << registration.get().id() << std::endl;
                        }
                        catch (const std::exception& e) {
                            std::cerr << e.what() << std::endl;
                            io.stop();
                            return;
                        }
                    });
                });
            });
        });

        std::cerr << "starting io service" << std::endl;
        io.run();
        std::cerr << "stopped io service" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
