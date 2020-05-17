About
=====

Clients communcating to each other via a server.

.. code-block:: text

   +------------------------------+
   |           server             |
   +-----o----------------o-------+
         |                |
         |                |
         |                |
   +-----o-----+  +-------o-------+
   | C-client  |  | Python-client |
   +-----------+  +---------------+

Build everything.

.. code-block:: text

   $ make -s

Start the server.

.. code-block:: text

   $ server/linux/server
   Server URI: tcp://127.0.0.1:6000
   Server started.
   Client <Linux connected.
   Number of connected clients: 1
   Client <Python> connected.
   Number of connected clients: 2
   Client <Async> connected.
   Number of connected clients: 3

Start three clients and send messages between them. Type a message and
press <Enter> to send it.

Client implemented in C for the Linux platform:

.. code-block:: text

   $ client/linux/client Linux
   Server URI: tcp://127.0.0.1:6000
   Connected to the server.
   Hello!
   <Linux> Hello!
   <Python> Hi!
   <Async> Hola!

Client implemented in Python:

.. code-block:: text

   $ make -s -C client/python
   Server URI: tcp://127.0.0.1:6000
   Connected to the server.
   <Linux> Hello!
   Hi!
   <Python> Hi!
   <Async> Hola!

Client implemented in C for the Async platform:

.. code-block:: text

   $ client/async/client Async
   Server URI: tcp://127.0.0.1:6000
   Connected to the server.
   Hello!
   <Linux> Hello!
   <Python> Hi!
   Hola!
   <Async> Hola!
