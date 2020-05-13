|buildstatus|_
|coverage|_
|codecov|_
|nala|_

⚽ Messi
========

Reliable message passing for distributed systems.

Architecture
------------

A `Messi` system consists of servers and clients. Once a client has
successfully connected to a server, messages defined in their protocol
specification can be sent between them.

Below is a sequence diagram showing typical communication between a
server and a client. The messages ``FooReq``, ``FooRsp`` and
``BarInd`` are defined in the protocol ``my_protocol``, which is
defined later in this document.

.. code-block:: text

   +--------+                               +--------+
   | client |                               | server |
   +--------+                               +--------+
       |             1. TCP connect              |
       |========================================>|
       |                2. ping                  |
       |---------------------------------------->|
       |                   pong                  |
       |<----------------------------------------|
       |               3. FooReq                 |
       |========================================>|
       |                  FooRsp                 |
       |<========================================|
       |               4. BarInd                 |
       |========================================>|
       |                  BarInd                 |
       |========================================>|
       .                                         .
       .                                         .
       .                                         .
       |                5. ping                  |
       |---------------------------------------->|
       .                                         .
       .                                         .
       .                                         .

   Legend:

     ---: Background communication. No user interaction needed.
     ===: User initiated communication.

1. The client connects to the server.

2. The client sends a ping message to the server, which responds with
   a pong message. This is done in the background. No user interaction
   needed.

3. The client sends ``FooReq`` to the server, which responds with
   ``FooRsp``.

4. The client sends ``BarInd`` twice to the server. No response is
   defined.

5. The client sends another ping message. This time the server does
   not respond, and the client application is notified that it has
   been disconnected from the server.

Messi protocol specification
----------------------------

All messages sent on the wire consists of a type, a size and optional
payload. This enables both streaming and packet based transport
protocols. The transport protocol must be reliable (guaranteed in
order delivery).

Type and size are in network byte order (big endian).

.. code-block:: text

   +---------+---------+-----------------+
   | 4b type | 4b size | <size>b payload |
   +---------+---------+-----------------+

   TYPE  SIZE  DESCRIPTION
   ------------------------------------------------------------------
      1     n  User message (server <-> client).

               Encoded "message ClientToServer" for client to server
               messages and "message ServerToClient" for server to
               client messages.

      2     0  Ping message (client --> client).
      3     0  Pong message (server --> client).

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

C source code
-------------

Generate server and client side C source code.

.. code-block:: text

   $ messi generate_c_source examples/hello_world/hello_world.proto

Use ``-p/--platform`` to select which platform to generate code
for. Planned platforms are Linux (using `epoll`_ and `timerfd`_) and
`async`_.

The generated code is **not** thread safe.

Client side
^^^^^^^^^^^

Per client.

.. code-block:: c

   void PROTO_client_init();   // Initialize given client.
   void PROTO_client_start();  // Connect to the server. Automatic reconnect.
   void PROTO_client_stop();   // Disconnect from the server. Call start to connect
                               // again.
   void PROTO_client_send();   // Send prepared message to server.

Per Linux client.

.. code-block:: c

   void PROTO_client_process();  // Process all pending events on given file
                                 // descriptor.

Per message.

.. code-block:: c

   void PROTO_client_init_MESSAGE(); // Initialize given message.

Server side
^^^^^^^^^^^

Per server.

.. code-block:: c

   void PROTO_server_init();        // Initialize given server.
   void PROTO_server_start();       // Start given server.
   void PROTO_server_stop();        // Stop given server.
   void PROTO_server_broadcast();   // Send prepared message to all clients.
   void PROTO_server_send();        // Send prepared message to given client.
   void PROTO_server_reply();       // Send prepared message to current client.
   void PROTO_server_disconnect();  // Disconnect given client.

Per Linux server.

.. code-block:: c

   void PROTO_server_process();  // Process all pending events on given file
                                 // descriptor.

Per message.

.. code-block:: c

   void PROTO_server_init_MESSAGE(); // Initialize given message.

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
