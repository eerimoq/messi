About
=====

Clients communcating to each other via a server.

.. code-block:: text

   +-----------------------------+
   |           server            |
   +---o----------o----------o---+
       |          |          |
       |          |          |
       |          |          |
   +---o---+  +---o---+  +---o---+
   | Erik  |  | Kalle |  |  Fia  |
   +-------+  +-------+  +-------+

Build everything.

.. code-block:: text

   $ make -s

Start the server.

.. code-block:: text

   $ server/linux/server
   Server URI: tcp://127.0.0.1:6000
   Server started.
   Number of connected clients: 1
   Client <C> connected.
   Number of connected clients: 2
   Client <Python> connected.

Start two clients and send messages between them. Type a message and
press <Enter> to send it.

Client implemented in C:

.. code-block:: text

   $ client/linux/client C
   Server URI: tcp://127.0.0.1:6000
   Connected to the server.
   Hello!
   <C> Hello!
   <Python> Hi!

Client implemented in Python:

.. code-block:: text

   $ make -s -C client/python
   Server URI: tcp://127.0.0.1:6000
   Connected to the server.
   <C> Hello!
   Hi!
   <Python> Hi!
