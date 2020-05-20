|buildstatus|_
|coverage|_
|codecov|_
|nala|_

⚽ Messi
========

Reliable message passing in distributed systems.

Highlights:

- Guaranteed in order message delivery.

- Client automatically reconnects when the server connection is lost.

- Protocols defined in Googles `Protocol Buffers`_ language version 3.

- Designed to work in resource constrained systems.

- `C` and `Python` source code generators.

Project homepage: https://github.com/eerimoq/messi

Installation
------------

.. code-block:: python

   $ pip install messi

Architecture
------------

A `Messi` system consists of servers and clients. Once a client has
successfully connected to a server, messages defined in their protocol
specification can be reliably sent between them.

`Messi` guarantees that all sent messages are delivered in order to
the peer. However, just as for TCP, any data/messages in flight when
the connection is lost will likely be lost.

Below is a sequence diagram showing typical communication between a
server and a client. The messages ``FooReq``, ``FooRsp``, ``BarInd``,
``FieReq`` and ``FieRsp`` are defined in the protocol ``my_protocol``,
which is defined later in this document.

Each step (1 to 6) is described in detail after the sequence diagram.

.. code-block:: text

                 +--------+                               +--------+
                 | client |                               | server |
                 +--------+                               +--------+
                     |             1. TCP connect              |
                     |========================================>|
      on_connected() |                2. ping                  | on_client_connected()
                     |---------------------------------------->|
                     |                   pong                  |
                     |<----------------------------------------|
              send() |               3. FooReq                 |
                     |========================================>|
                     |                  FooRsp                 | reply()
                     |<========================================|
              send() |               4. BarInd                 |
                     |========================================>|
              send() |                  BarInd                 |
                     |========================================>|
                     |               5. FieReq                 | send()
                     |<========================================|
              send() |                  FieRsp                 |
                     |========================================>|
                     .                                         .
                     .                                         .
                     .                                         .
                     |                6. ping                  |
                     |---------------------------------------->|
                     .                                         .
   on_disconnected() .                                         . on_client_disconnected()
                     .                                         .

   Legend:

     ---: Background communication. No user interaction needed.

     ===: User initiated communication.

Step by step description:

1. The client connects to the server. ``on_connected()`` and
   ``on_client_connected()`` are called to notify the user that the
   connection has been established.

2. The client sends a ping message to the server, which responds with
   a pong message. This is done in the background. No user interaction
   needed.

3. The client sends ``FooReq`` to the server, which responds with
   ``FooRsp``.

4. The client sends ``BarInd`` twice to the server. No response is
   defined.

5. The server sends ``FieReq`` to the client, which responds with
   ``FieRsp``.

6. The client sends another ping message. This time the server does
   not respond. ``on_disconnected()`` and ``on_client_disconnected()``
   are called to notify the user about the disconnection.

Messi protocol specification
----------------------------

All messages sent on the wire consists of a type, a size and optional
payload. This enables both streaming and packet based transport
protocols. The transport protocol must be reliable (guaranteed in
order delivery).

Type and size are in network byte order (big endian).

.. code-block:: text

   +---------+---------+-----------------+
   | 1b type | 3b size | <size>b payload |
   +---------+---------+-----------------+

   TYPE  SIZE  DESCRIPTION
   ------------------------------------------------------------------
      1     n  Client to server user message (client --> server).

               Encoded "message ClientToServer" messages.

      2     n  Server to client user message (server --> client).

               Encoded "message ServerToClient" messages.

      3     0  Ping message (client --> server).
      4     0  Pong message (server --> client).

User messages
^^^^^^^^^^^^^

User messages are defined in Googles `Protocol Buffers`_ language
version 3.

Here is an example defining a protocol called ``my_protocol``. The two
messages ``ClientToServer`` and ``ServerToClient`` must be present in
every protocol specification. ``ClientToServer`` contains all messages
sent from clients to servers, and ``ServerToClient`` contains all
messages sent from servers to clients.

.. code-block:: protobuf

   syntax = "proto3";

   // The protocol name.
   package my_protocol;

   // Messages sent from client to server.
   message ClientToServer {
       oneof messages {
           FooReq foo_req = 1;
           BarInd bar_ind = 2;
           FieRsp fie_rsp = 3;
       }
   }

   // Messages sent from server to client.
   message ServerToClient {
       oneof messages {
           FooRsp foo_rsp = 1;
           FieReq fie_req = 2;
       }
   }

   // Message definitions.
   message FooReq {
   }

   message FooRsp {
   }

   message BarInd {
   }

   message FieReq {
   }

   message FieRsp {
   }

Ping and pong messages
^^^^^^^^^^^^^^^^^^^^^^

A client pings its server periodically. A client will close the
connection and report an error if the server does not answer with pong
within given time. Likewise, the server will close the connection and
report an error if it does not receive ping within given time.

The ping-pong mechanism is only used if the transport layer does not
provide equivalent functionality.

Error handling
--------------

`Messi` aims to minimize the amount of error handling code in the user
application. Almost all functions always succeeds from the caller
point of view. For example, ``PROTO_client_send()`` returns
``void``. If an error occurs, likely a connection issue, the
disconnect callback is called to notify the user that the connection
was dropped.

C source code
-------------

Generate server and client side C source code.

.. code-block:: text

   $ messi generate_c_source examples/chat/chat.proto

Use ``-p/--platform`` to select which platform to generate code
for.

Supported platforms:

- Linux TCP, using `epoll`_ and `timerfd`_.

- The `async`_ framework (client only).

The generated code is **not** thread safe.

Known limitations:

- The connection is immediately dropped if ``write()`` does not accept
  exaxtly given amount of bytes. Buffering of remaining data may be
  added at some point.

Client side
^^^^^^^^^^^

``PROTO`` is replaced by the protocol name and ``MESSAGE`` is replaced
by the message name.

Per client:

.. code-block:: c

   void PROTO_client_init();   // Initialize given client.
   void PROTO_client_start();  // Connect to the server. The connected callback is
                               // called once connected. Automatic reconnect if
                               // disconnected.
   void PROTO_client_stop();   // Disconnect from the server. Call start to connect
                               // again.
   void PROTO_client_send();   // Send prepared message to the server.

   typedef void (*PROTO_client_on_connected_t)();    // Callback called when connected
                                                     // to the server.
   typedef void (*PROTO_client_on_disconnected_t)(); // Callback called when disconnected
                                                     // from the server.

Per Linux client:

.. code-block:: c

   void PROTO_client_process();  // Process all pending events on given file
                                 // descriptor (if it belongs to given client).

Per message:

.. code-block:: c

   void PROTO_client_init_MESSAGE(); // Prepare given message. Call send to send it.

   typedef void (*PROTO_client_on_MESSAGE_t)(); // Callback called when given message
                                                // is received from the server.

Below is pseudo code using the Linux client side generated code from
the protocol ``my_protocol``, defined earlier in the document. The
complete implementation is found in
`examples/my_protocol/client/linux/main.c`_.

.. code-block:: c

   static void on_connected(struct my_protocol_client_t *self_p)
   {
       my_protocol_client_init_foo_req(self_p);
       my_protocol_client_send(self_p);
   }

   static void on_disconnected(struct my_protocol_client_t *self_p)
   {
   }

   static void on_foo_rsp(struct my_protocol_client_t *self_p,
                          struct my_protocol_foo_rsp_t *message_p)
   {
       my_protocol_client_init_bar_ind(self_p);
       my_protocol_client_send(self_p);
       my_protocol_client_send(self_p);
   }

   static void on_fie_req(struct my_protocol_client_t *self_p,
                          struct my_protocol_fie_req_t *message_p)
   {
       my_protocol_client_init_fie_rsp(self_p);
       my_protocol_client_send(self_p);
   }

   void main()
   {
       struct my_protocol_client_t client;
       ...
       my_protocol_client_init(&client,
                               ...
                               "tcp://127.0.0.1:7840",
                               ...
                               on_connected,
                               on_disconnected,
                               on_foo_rsp,
                               on_fie_req,
                               ...);
       my_protocol_client_start(&client);

       while (true) {
           epoll_wait(epoll_fd, &event, 1, -1);
           my_protocol_client_process(&client, event.data.fd, event.events);
       }
   }

Server side
^^^^^^^^^^^

``PROTO`` is replaced by the protocol name and ``MESSAGE`` is replaced
by the message name.

Per server:

.. code-block:: c

   void PROTO_server_init();        // Initialize given server.
   void PROTO_server_start();       // Start accepting clients.
   void PROTO_server_stop();        // Disconnect any clients and stop accepting new
                                    // clients.
   void PROTO_server_send();        // Send prepared message to given client.
   void PROTO_server_reply();       // Send prepared message to current client.
   void PROTO_server_broadcast();   // Send prepared message to all clients.
   void PROTO_server_disconnect();  // Disconnect current or given client.

   typedef void (*PROTO_server_on_client_connected_t)();    // Callback called when a
                                                            // client has connected.
   typedef void (*PROTO_server_on_client_disconnected_t)(); // Callback called when a
                                                            // client is disconnected.

Per Linux server:

.. code-block:: c

   void PROTO_server_process();  // Process all pending events on given file
                                 // descriptor (if it belongs to given server).

Per message:

.. code-block:: c

   void PROTO_server_init_MESSAGE(); // Prepare given message. Call send, reply or
                                     // broadcast to send it.

   typedef void (*PROTO_server_on_MESSAGE_t)(); // Callback called when given message
                                                // is received from given client.

Below is pseudo code using the Linux server side generated code from
the protocol ``my_protocol``, defined earlier in the document. The
complete implementation is found in
`examples/my_protocol/server/linux/main.c`_.

.. code-block:: c

   static void on_foo_req(struct my_protocol_server_t *self_p,
                          struct my_protocol_server_client_t *client_p,
                          struct my_protocol_foo_req_t *message_p)
   {
       my_protocol_server_init_foo_rsp(self_p);
       my_protocol_server_reply(self_p);
   }

   static void on_bar_ind(struct my_protocol_server_t *self_p,
                          struct my_protocol_server_client_t *client_p,
                          struct my_protocol_bar_ind_t *message_p)
   {
       my_protocol_server_init_fie_req(self_p);
       my_protocol_server_reply(self_p);
   }

   static void on_fie_rsp(struct my_protocol_server_t *self_p,
                          struct my_protocol_server_client_t *client_p,
                          struct my_protocol_fie_rsp_t *message_p)
   {
   }

   void main()
   {
       struct my_protocol_server_t server;
       ...
       my_protocol_server_init(&server,
                               "tcp://127.0.0.1:7840",
                               ...
                               on_foo_req,
                               on_bar_ind,
                               on_fie_rsp,
                               ...);
       my_protocol_server_start(&server);

       while (true) {
           epoll_wait(epoll_fd, &event, 1, -1);
           my_protocol_server_process(&server, event.data.fd, event.events);
       }
   }

Python source code
------------------

Generate client side Python source code.

.. code-block:: text

   $ messi generate_py_source examples/chat/chat.proto

See `tests/files/chat/chat_client.py`_ for the generated code and
`examples/chat/client/python/main.py`_ for example usage.

Similar solutions
-----------------

- `gRPC`_ with bidirectional streaming.

.. |buildstatus| image:: https://travis-ci.com/eerimoq/messi.svg?branch=master
.. _buildstatus: https://travis-ci.com/eerimoq/messi

.. |coverage| image:: https://coveralls.io/repos/github/eerimoq/messi/badge.svg?branch=master
.. _coverage: https://coveralls.io/github/eerimoq/messi

.. |codecov| image:: https://codecov.io/gh/eerimoq/messi/branch/master/graph/badge.svg
.. _codecov: https://codecov.io/gh/eerimoq/messi

.. |nala| image:: https://img.shields.io/badge/nala-test-blue.svg
.. _nala: https://github.com/eerimoq/nala

.. _epoll: https://en.wikipedia.org/wiki/Epoll

.. _timerfd: http://man7.org/linux/man-pages/man2/timerfd_settime.2.html

.. _async: https://github.com/eerimoq/async

.. _Protocol Buffers: https://developers.google.com/protocol-buffers/docs/proto3

.. _examples/my_protocol/client/linux/main.c: https://github.com/eerimoq/messi/blob/master/examples/my_protocol/client/linux/main.c

.. _examples/my_protocol/server/linux/main.c: https://github.com/eerimoq/messi/blob/master/examples/my_protocol/server/linux/main.c

.. _examples/chat/client/python/main.py: https://github.com/eerimoq/messi/blob/master/examples/chat/client/python/main.py

.. _tests/files/chat/chat_client.py: https://github.com/eerimoq/messi/blob/master/tests/files/chat/chat_client.py

.. _gRPC: https://grpc.io/
