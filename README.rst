|buildstatus|_
|coverage|_
|nala|_

Messager
========

Reliable message passing in distributed systems.

Architecture
------------

Zero or more clients can connect to a server.

API
---

Generate server and client side C source code.

.. code-block:: text

   $ kamari generate_c_source examples/hello_world/hello_world.proto

Client side
^^^^^^^^^^^

Per client.

.. code-block:: c

   void PROTO_client_init();       // Initialize given client.
   void PROTO_client_connect();    // Connect to the server.
   void PROTO_client_disconnect(); // Disconnect from the server.
   void PROTO_client_send();       // Send prepared message to server.

Per message.

.. code-block:: c

   void PROTO_client_init_MESSAGE(); // Initialize given message.

Server side
^^^^^^^^^^^

Per server.

.. code-block:: c

   void PROTO_server_init();          // Initialize given server.
   void PROTO_server_serve_forever(); // Serve clients forever.
   void PROTO_server_send();          // Send prepared message to given client.
   void PROTO_server_reply();         // Send prepared message to current client.
   void PROTO_server_broadcast();     // Send prepared message to all clients.

Per message.

.. code-block:: c

   void PROTO_server_init_MESSAGE(); // Initialize given message.

Protocol
--------

.. code-block:: text

   +---------+---------+-----------------+
   | 4b type | 4b size | <size>b payload |
   +---------+---------+-----------------+

   TYPE  SIZE  DESCRIPTION
   ------------------------------------------------
      1     n  User message.
      2     0  Ping.
      3     0  Pong.

Messages are defined using Googles Protocol Buffers language. All
messages in a protocol are part of a single oneof message.

Clients pings the server periodically. A client will close the
connection and report an error if the server does not answer with pong
within given time. Likewise, the server will close the connection and
report an error if it does not receive ping within given time.

The ping-pong mechanism is only used if the transport layer does not
provide equivalent functionality.

.. |buildstatus| image:: https://travis-ci.org/eerimoq/messager.svg?branch=master
.. _buildstatus: https://travis-ci.org/eerimoq/messager

.. |coverage| image:: https://coveralls.io/repos/github/eerimoq/messager/badge.svg?branch=master
.. _coverage: https://coveralls.io/github/eerimoq/messager

.. |nala| image:: https://img.shields.io/badge/nala-test-blue.svg
.. _nala: https://github.com/eerimoq/nala
