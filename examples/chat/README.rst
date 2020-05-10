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

   $ server/server
   Server started.
   Client <Erik> connected.
   Client <Kalle> connected.
   Client <Fia> connected.

Start three clients and send messages to each other. Type a message
and press <Enter> to send it. The text is displayed once the message
is received from the server.

.. code-block:: text

   $ client/client Erik
   Connceted to the server.
   <Erik> Hello all!
   <Fia> Hi!
   <Kalle> Howdy

.. code-block:: text

   $ client/client Kalle
   Connceted to the server.
   <Erik> Hello all!
   <Fia> Hi!
   <Kalle> Howdy

.. code-block:: text

   $ client/client Fia
   Connceted to the server.
   <Erik> Hello all!
   <Fia> Hi!
   <Kalle> Howdy
