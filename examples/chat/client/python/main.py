import threading
import argparse
import asyncio

from chat_client import ChatClient


class Client(ChatClient):

    def __init__(self, user, uri):
        super().__init__(uri)
        self._user = user

    async def on_connected(self):
        message = self.init_connect_req()
        message.user = self._user
        self.send()

    async def on_disconnected(self):
        print("Disconnected from the server.")

    async def on_connect_rsp(self, message):
        print("Connected to the server.");

    async def on_message_ind(self, message):
        print(f"<{message.user}> {message.text}");

    async def on_input(self, text):
        message = self.init_message_ind()
        message.user = self._user
        message.text = text
        self.send()


class ClientThread(threading.Thread):

    def __init__(self, loop, client):
        super().__init__()
        self._loop = loop
        self._client = client
        self.daemon = True

    def run(self):
        asyncio.set_event_loop(self._loop)
        self._loop.run_until_complete(self._client.start())
        self._loop.run_forever()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--uri', default='tcp://127.0.0.1:6000')
    parser.add_argument('user')

    args = parser.parse_args()

    print(f"Server URI: {args.uri}")

    loop = asyncio.new_event_loop()
    client = Client(args.user, args.uri)
    client_thread = ClientThread(loop, client)
    client_thread.start()

    while True:
        text = input('')
        asyncio.run_coroutine_threadsafe(client.on_input(text), loop)


if __name__ == '__main__':
    main()
