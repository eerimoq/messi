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

   $ messi generate_c_source examples/my_protocol/my_protocol.proto

Use ``-p/--platform`` to select which platform to generate code
for.

Supported platforms:

- Linux, using TCP sockets, `epoll`_ and `timerfd`_.

- The `async`_ framework (client only).

The generated code is **not** thread safe.

Known limitations:

- The connection is immediately dropped if ``write()`` does not accept
  exaxtly given amount of bytes. Buffering of remaining data may be
  added at some point.

Linux client side
^^^^^^^^^^^^^^^^^

See `tests/files/my_protocol/linux/my_protocol_client.h`_ for the
generated code and `examples/my_protocol/client/linux/main.c`_ for
example usage.

Linux server side
^^^^^^^^^^^^^^^^^

See `tests/files/my_protocol/linux/my_protocol_server.h`_ for the
generated code and `examples/my_protocol/server/linux/main.c`_ for
example usage.

Async client side
^^^^^^^^^^^^^^^^^

See `tests/files/my_protocol/async/my_protocol_client.h`_ for the
generated code and `examples/my_protocol/client/async/main.c`_ for
example usage.

Python source code
------------------

Generate client side Python source code.

.. code-block:: text

   $ messi generate_py_source examples/my_protocol/my_protocol.proto

Client side
^^^^^^^^^^^

See `tests/files/my_protocol/my_protocol_client.py`_ for the generated
code and `examples/my_protocol/client/python/main.py`_ for example
usage.

Server side
^^^^^^^^^^^

Not yet implemented.

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

.. _tests/files/my_protocol/my_protocol_client.py: https://github.com/eerimoq/messi/blob/master/tests/files/my_protocol/my_protocol_client.py

.. _examples/my_protocol/client/python/main.py: https://github.com/eerimoq/messi/blob/master/examples/my_protocol/client/python/main.py

.. _tests/files/my_protocol/linux/my_protocol_client.h: https://github.com/eerimoq/messi/blob/master/tests/files/my_protocol/linux/my_protocol_client.h

.. _examples/my_protocol/client/linux/main.c: https://github.com/eerimoq/messi/blob/master/examples/my_protocol/client/linux/main.c

.. _tests/files/my_protocol/async/my_protocol_client.h: https://github.com/eerimoq/messi/blob/master/tests/files/my_protocol/async/my_protocol_client.h

.. _examples/my_protocol/client/async/main.c: https://github.com/eerimoq/messi/blob/master/examples/my_protocol/client/async/client.c

.. _tests/files/my_protocol/linux/my_protocol_server.h: https://github.com/eerimoq/messi/blob/master/tests/files/my_protocol/linux/my_protocol_server.h

.. _examples/my_protocol/server/linux/main.c: https://github.com/eerimoq/messi/blob/master/examples/my_protocol/server/linux/main.c

.. _gRPC: https://grpc.io/
