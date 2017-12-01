
import argparse
import socket
import threading
import http.server


def requestHandler(client):

        client[0].settimeout(None)

        request = client[0].recv(1024)

        requestedServer = (str(request).split(' ')[1].split(':')[1].replace("/", ""), 80)
        try:
            requestedServer[1] = str(request).split(' ')[1].split(':')[2]
        except:
            pass

        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.connect(requestedServer)

        if(args.timeout > 0):
            server.settimeout(args.timeout)
        else:
            server.settimeout(None)

        while len(ret) > 0:
            print("recieving packet")

            print(len(ret))

            if str(request)[2:9] != "CONNECT":

                client.send(ret)

                ret = server.recv(1024)

        client[0].close()
        server.close()


def main(args):


    server = 0
    currentServers = []
    print("Proxy Server's IP:Port  -> ", socket.gethostbyname(socket.gethostname()) , ":", args.port)

    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clientSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    clientSocket.bind((socket.gethostbyname(socket.gethostname()), args.port))
    clientSocket.listen(1)

    while True:
        client = clientSocket.accept()

        if threading.activeCount() <= args.numworkers:
            t = threading.Thread(target = requestHandler, args = (client))
            t.start()


    clientSocket.close()
    print("closing")
    print(threading.active_count())




parser = argparse.ArgumentParser(description = "Proxy Server", add_help = False)
parser.add_argument('-h', '--help', action ="store_true", default = False)
parser.add_argument('-v', '--version', action ="store_true", default = False)
parser.add_argument('-p', '--port', type=int)
parser.add_argument('-n', '--numworkers', type=int, default = 10)
parser.add_argument('-t', '--timeout', type=int, default = -1)
parser.add_argument('-l', '--log', action ="store_true", default = False)

args = parser.parse_args()
main(args)
