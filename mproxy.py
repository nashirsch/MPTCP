
import argparse
import socket
import threading
import http.server


def requestHandler(client, server, request):
        print(3)
        print(request)
        server.send(request)

        while True:
            print("recieving packet")
            ret = server.recv(8192)
            print(len(ret))
            client.send(ret)

            if len(ret) == 0:
                print("breaking")
                break


def main(args):

    currentServers = []
    print("Proxy Server's IP:Port  -> ", socket.gethostbyname(socket.gethostname()) , ":", args.port)

    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clientSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    clientSocket.bind((socket.gethostbyname(socket.gethostname()), args.port))
    clientSocket.listen(1)


    client = clientSocket.accept()
    client[0].setblocking(1)

    i = 10
    while i > 0:
        i -= 1

        request = ""
        request = client[0].recv(1024)

        if(len(request) < 4):
            continue

        print(request)

        requestedServer = (str(request).split(' ')[1].split(':')[0],
                           int(str(request).split(' ')[1].split(':')[1]))

        print(requestedServer)

        server = [i[0] for i in currentServers if i[1] is requestedServer]

        if (requestedServer not in server[1] for server in currentServers):
            if str(request)[2:9] == "CONNECT":
                server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server.connect(requestedServer)

                if(args.timeout > 0):
                    server.settimeout(args.timeout)
                else:
                    server.settimeout(None)

                currentServers.append((server, requestedServer))
            else:
                print("Error: Non-connect method on unconnected server")
                exit()

        print("1")
        if threading.activeCount() <= args.numworkers:
            print("2")
            t = threading.Thread(target = requestHandler, args = (client[0], server, request))
            t.start()

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
