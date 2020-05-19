import asyncio
import logging
import bitstruct

from messi import MessageType
from messi import parse_tcp_uri
import chat_pb2


LOGGER = logging.getLogger(__name__)

CF_HEADER = bitstruct.compile('u8u24')


class ChatClient:

    def __init__(self, uri):
        self._address, self._port = parse_tcp_uri(uri)
        self._uri = uri
        self._reader = None
        self._writer = None
        self._task = None
        self._keep_alive_task = None
        self._pong_event = None
        self._output = None

    async def start(self):
        if self._task is None:
            self._task = asyncio.create_task(self._main())

    async def stop(self):
        if self._task is not None:
            self._task.cancel()

    def send(self):
        encoded = self._output.SerializeToString()
        header = CF_HEADER.pack(MessageType.CLIENT_TO_SERVER_USER, len(encoded))
        self._writer.write(header + encoded)

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

    async def _main(self):
        while True:
            await self._connect()
            await self.on_connected()
            self._pong_event = asyncio.Event()
            self._keep_alive_task = asyncio.create_task(self._keep_alive_main())

            try:
                await self._reader_loop()
            except Exception as e:
                pass

            self._keep_alive_task.cancel()

    async def _connect(self):
        while True:
            try:
                self._reader, self._writer = await asyncio.open_connection(
                    self._address,
                    self._port)
                break
            except ConnectionRefusedError:
                LOGGER.info("Failed to connect to '%s:%d'.",
                            self._address,
                            self._port)
                await asyncio.sleep(1)

    async def _handle_user_message(self, payload):
        message = chat_pb2.ServerToClient()
        message.ParseFromString(payload)
        choice = message.WhichOneof('messages')

        if choice == 'connect_rsp':
            await self.on_connect_rsp(message.connect_rsp)
        elif choice == 'message_ind':
            await self.on_message_ind(message.message_ind)

    def _handle_pong(self):
        self._pong_event.set()

    async def _reader_loop(self):
        while True:
            header = await self._reader.readexactly(4)
            message_type, size = CF_HEADER.unpack(header)
            payload = await self._reader.readexactly(size)

            if message_type == MessageType.SERVER_TO_CLIENT_USER:
                await self._handle_user_message(payload)
            elif message_type == MessageType.PONG:
                self._handle_pong()

    async def _keep_alive_main(self):
        while True:
            await asyncio.sleep(2)
            self._pong_event.clear()
            self._writer.write(CF_HEADER.pack(MessageType.PING, 0))
            await asyncio.wait_for(self._pong_event.wait(), 3)
