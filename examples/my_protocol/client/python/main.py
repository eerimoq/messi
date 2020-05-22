import sys
import asyncio

from my_protocol_client import MyProtocolClient


class Client(MyProtocolClient):

    async def on_connected(self):
        print("Connected. Sending FooReq.")

        self.init_foo_req()
        self.send()

    async def on_disconnected(self):
        print("Disconnected. Exiting.")

        sys.exit(0)

    async def on_foo_rsp(self, message):
        print("Got FooRsp. Sending BarInd twice.")

        self.init_bar_ind()
        self.send()
        self.send()

    async def on_fie_req(self, message):
        print("Got FieReq. Sending FieRsp.")

        self.init_fie_rsp()
        self.send()


async def main():
    client = Client('tcp://127.0.0.1:7840')
    client.start()

    while True:
        await asyncio.sleep(10)


if __name__ == '__main__':
    asyncio.run(main())
