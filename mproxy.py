
import argparse
import socket
import threading
import http.server


def requestHandler(client, server, request):
        print(3)
        print(request)
        server.send(request)

        ret = server.recv(8192)
        while len(ret) > 1:
            print("recieving packet")

            ret = server.recv(8192)

            print(len(ret))

            client.send(ret)

            ret = server.recv(8192)


def main(args):


    server = 0
    currentServers = []
    print("Proxy Server's IP:Port  -> ", socket.gethostbyname(socket.gethostname()) , ":", args.port)

    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clientSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    clientSocket.bind((socket.gethostbyname(socket.gethostname()), args.port))
    clientSocket.listen(1)


    client = clientSocket.accept()
    client[0].setblocking(1)
    client[0].settimeout(None)

    request = client[0].recv(1024)
    while len(request) > 1:

        print(request)

        if str(request)[2:9] != "CONNECT":

            requestedServer = (str(request).split(' ')[1].split(':')[1].replace("/", ""), 80)

            print(requestedServer)

            if (requestedServer not in serv[1] for serv in currentServers):

                server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server.connect(requestedServer)

                if(args.timeout > 0):
                    server.settimeout(args.timeout)
                else:
                    server.settimeout(None)

                currentServers.append((server, requestedServer))

            else:
                server = [i[0] for i in currentServers if i[1] is requestedServer]

            if threading.activeCount() <= args.numworkers:
                t = threading.Thread(target = requestHandler, args = (client[0], server, request))
                t.start()

        request = client[0].recv(1024)

    print("closing")
    client[0].close()




parser = argparse.ArgumentParser(description = "Proxy Server", add_help = False)
parser.add_argument('-h', '--help', action ="store_true", default = False)
parser.add_argument('-v', '--version', action ="store_true", default = False)
parser.add_argument('-p', '--port', type=int)
parser.add_argument('-n', '--numworkers', type=int, default = 10)
parser.add_argument('-t', '--timeout', type=int, default = -1)
parser.add_argument('-l', '--log', action ="store_true", default = False)

args = parser.parse_args()
main(args)
