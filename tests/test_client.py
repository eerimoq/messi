import logging
import asyncio
import unittest
import shutil

import messi
messi.py_source.generate_files('both', [], '.', ['tests/files/chat/chat.proto'])
shutil.copy('tests/files/chat/chat_pb2.py', '.')
import chat_client


def create_tcp_uri(listener):
    address, port = listener.sockets[0].getsockname()

    return f'tcp://{address}:{port}'


class ChatClient(chat_client.ChatClient):

    def __init__(self, uri, keep_alive_interval=2):
        super().__init__(uri, keep_alive_interval)
        self.connected_event = asyncio.Event()

    async def on_connected(self):
        message = self.init_connect_req()
        message.user = 'Erik'
        self.send()

    async def on_disconnected(self):
        print("Disconnected from the server.")

    async def on_connect_rsp(self, message):
        self.connected_event.set()

    async def on_message_ind(self, message):
        print(f"<{message.user}> {message.text}");


async def server_main(listener):
    async with listener:
        try:
            await listener.serve_forever()
        except asyncio.CancelledError:
            pass


CONNECT_REQ = b'\x01\x00\x00\x08\n\x06\n\x04Erik'
CONNECT_RSP = b'\x02\x00\x00\x02\x0a\x00'
PING = b'\x03\x00\x00\x00'
PONG = b'\x04\x00\x00\x00'


class ClientTest(unittest.TestCase):

    def setUp(self):
        self.connect_count = 0

    async def read_connect_req(self, reader):
        connect_req = await reader.readexactly(12)
        self.assertEqual(connect_req, CONNECT_REQ)

    async def read_ping(self, reader):
        ping = await reader.readexactly(4)
        self.assertEqual(ping, PING)

    def test_connect_disconnect(self):
        asyncio.run(self.connect_disconnect())

    async def connect_disconnect(self):
        async def on_client_connected(reader, writer):
            await self.read_connect_req(reader)
            writer.write(CONNECT_RSP)
            writer.close()

        listener = await asyncio.start_server(on_client_connected, 'localhost', 0)

        async def client_main():
            client = ChatClient(create_tcp_uri(listener))
            client.start()
            await asyncio.wait_for(client.connected_event.wait(), 2)
            client.stop()
            listener.close()

        await asyncio.wait_for(
            asyncio.gather(server_main(listener), client_main()), 2)

    def test_reconnect_on_missing_pong(self):
        asyncio.run(self.reconnect_on_missing_pong())

    async def reconnect_on_missing_pong(self):

        async def on_client_connected(reader, writer):
            if self.connect_count == 0:
                await self.read_connect_req(reader)
                await self.read_ping(reader)
                # Do not send pong. Wait for reconnect.
                self.connect_count += 1
            elif self.connect_count == 1:
                await self.read_connect_req(reader)
                writer.write(CONNECT_RSP)

            writer.close()

        listener = await asyncio.start_server(on_client_connected, 'localhost', 0)

        async def client_main():
            client = ChatClient(create_tcp_uri(listener))
            client.start()
            await asyncio.wait_for(client.connected_event.wait(), 10)
            client.stop()
            listener.close()

        await asyncio.wait_for(
            asyncio.gather(server_main(listener), client_main()), 10)

    def test_keep_alive(self):
        asyncio.run(self.keep_alive())

    async def keep_alive(self):

        async def on_client_connected(reader, writer):
            await self.read_connect_req(reader)
            await self.read_ping(reader)
            writer.write(PONG)
            await self.read_ping(reader)
            writer.write(CONNECT_RSP)
            writer.close()

        listener = await asyncio.start_server(on_client_connected, 'localhost', 0)

        async def client_main():
            client = ChatClient(create_tcp_uri(listener), keep_alive_interval=1)
            client.start()
            await asyncio.wait_for(client.connected_event.wait(), 10)
            client.stop()
            listener.close()

        await asyncio.wait_for(
            asyncio.gather(server_main(listener), client_main()), 10)

    def test_parse_tcp_uri(self):
        self.assertEqual(chat_client.parse_tcp_uri('tcp://127.0.0.1:6000'),
                         ('127.0.0.1', 6000));

    def test_parse_uri_tcp_any_address(self):
        self.assertEqual(chat_client.parse_tcp_uri('tcp://:6000'),
                         ('', 6000))

    def test_parse_uri_tcp_bad_scheme(self):
        with self.assertRaises(ValueError) as cm:
            chat_client.parse_tcp_uri('foo://127.0.0.1:6000')

        self.assertEqual(
            str(cm.exception),
            "Expected URI on the form tcp://<host>:<port>, but got "
            "'foo://127.0.0.1:6000'.")

    def test_parse_uri_tcp_no_colon_before_port(self):
        with self.assertRaises(ValueError) as cm:
            chat_client.parse_tcp_uri('tcp://127.0.0.1 6000')

        self.assertEqual(
            str(cm.exception),
            "Expected URI on the form tcp://<host>:<port>, but got "
            "'tcp://127.0.0.1 6000'.")


logging.basicConfig(level=logging.DEBUG)
