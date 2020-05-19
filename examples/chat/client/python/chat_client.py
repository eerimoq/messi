import asyncio
import bitstruct

import chat_pb2
from messi import MessageType


class ChatClient:

    def __init__(self, uri):
        self._uri = uri
        self._reader = None
        self._writer = None
        self._reader_task = None
        self._keep_alive_task = None
        self._pong_event = None
        self._output = None

    async def start(self):
        self._reader, self._writer = await asyncio.open_connection(
            '127.0.0.1',
            6000)
        self._pong_event = asyncio.Event()
        self._reader_task = asyncio.create_task(self._reader_main())
        self._keep_alive_task = asyncio.create_task(self._keep_alive_main())
        await self.on_connected()

    async def stop(self):
        self._writer.close()
        self._reader = None
        self._writer = None

    def send(self):
        encoded = self._output.SerializeToString()
        header = bitstruct.pack('>u8u24',
                                MessageType.CLIENT_TO_SERVER_USER,
                                len(encoded))
        self._writer.write(header)
        self._writer.write(encoded)

    async def on_connected(self):
        pass

    async def on_disconnected(self):
        pass

    async def on_connect_rsp(self, message):
        pass

    async def on_message_ind(self, message):
        pass

    def init_connect_req(self):
        self._output = chat_pb2.ClientToServer()

        return self._output.connect_req

    def init_message_ind(self):
        self._output = chat_pb2.ClientToServer()

        return self._output.message_ind

    async def handle_user_message(self, payload):
        message = chat_pb2.ServerToClient()
        message.ParseFromString(payload)
        choice = message.WhichOneof('messages')

        if choice == 'connect_rsp':
            await self.on_connect_rsp(message.connect_rsp)
        elif choice == 'message_ind':
            await self.on_message_ind(message.message_ind)

    def handle_pong(self):
        self._pong_event.set()

    async def _reader_main(self):
        while True:
            header = await self._reader.readexactly(4)
            message_type, size = bitstruct.unpack('>u8u24', header)
            payload = await self._reader.readexactly(size)

            if message_type == MessageType.SERVER_TO_CLIENT_USER:
                await self.handle_user_message(payload)
            elif message_type == MessageType.PONG:
                self.handle_pong()

    async def _keep_alive_main(self):
        while True:
            await asyncio.sleep(2)
            self._pong_event.clear()
            self._writer.write(bitstruct.pack('>u8u24', MessageType.PING, 0))
            await asyncio.wait_for(self._pong_event.wait(), 3)
