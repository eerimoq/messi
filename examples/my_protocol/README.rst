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
                     |               TCP disconnect            | exit()
                     |<========================================|
   on_disconnected() |                                         |
              exit() |                                         |
