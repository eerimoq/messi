class MessageType:

    CLIENT_TO_SERVER_USER = 1
    SERVER_TO_CLIENT_USER = 2
    PING = 3
    PONG = 4


def parse_tcp_uri(uri):
    """Parse tcp://<host>:<port>.

    """
    
    if uri[:6] != 'tcp://':
        raise Expection('Bad TCP URI.')

    address, port = uri[6:].split(':')

    return address, int(port)
