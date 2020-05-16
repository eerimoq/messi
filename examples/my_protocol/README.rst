About
=====

Similar to the my_protocol example in ../../README.rst.

Sequence diagram of connection setup and user messages sent in this
example.

.. code-block:: text

                 +--------+                               +--------+
                 | client |                               | server |
                 +--------+                               +--------+
                     |                TCP connect              |
                     |========================================>|
      on_connected() |                                         | on_client_connected()
              send() |                  FooReq                 |
                     |========================================>|
                     |                  FooRsp                 | reply()
                     |<========================================|
              send() |                  BarInd                 |
                     |========================================>|
              send() |                  BarInd                 |
                     |========================================>|
                     |                  FieReq                 | reply()
                     |<========================================|
              send() |                  FieRsp                 |
                     |========================================>|
                     |               TCP disconnect            | disconnect()
                     |<========================================|
   on_disconnected() |                                         | exit()
              exit() |                                         |

Build everything.

.. code-block:: text

   $ make -s

Start the server.

.. code-block:: text

   $ server/linux/server
   Server started.
   Got FooReq. Sending FooRsp.
   Got BarInd.
   Got BarInd. Sending FieReq.
   Got FieRsp. Disconnecting the client and exiting.

Start the client.

.. code-block:: text

   $ client/linux/client
   Connected. Sending FooReq.
   Got FooRsp. Sending BarInd twice.
   Got FieReq. Sending FieRsp.
   Disconnected. Exiting.
