import hello_world


class Client(hello_world.Client):
    
    def send_greeting(self, text):
        print(f"Sending '{text}'.")

        message = self.init_greeting_ind()
        message.text = text
        self.send()

    def on_disconnected(self):
        print("Disconnected.")
    

def main():
    client = Client()

    while True:
        client.connect("tcp://127.0.0.1:6000")
        print("Connected to the server.")

        client.send_greeting("Hello!")
        time.sleep(1)
        client.send_greeting("Hi!")
        time.sleep(1)
        client.send_greeting("Hi again!")
        
        print("Disconnecting from the server.")
        client.disconnect()

        print("Waiting 3 seconds before connecting again.")
        time.sleep(3)
