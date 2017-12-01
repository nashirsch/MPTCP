
import argparse
import socket
import threading


class thread(threading.Thread):
    def __init__(self, client, server, request):
        threading.Thread.__init__(self)
        self.client = client
        self.server = server
        self.request = request
        pass
    def run(self):
        print("running")
        self.server.send(str.encode(self.request))

        ret = self.server.recv(1024)
        print(ret)
        self.client.send(ret)





def main(args):


    currentServers = []
    print("Proxy Server's IP:Port  -> ", socket.gethostbyname(socket.gethostname()) , ":", args.port)

    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clientSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    clientSocket.bind((socket.gethostbyname(socket.gethostname()), args.port))
    clientSocket.listen(1)


    client = clientSocket.accept() #client = (connSd, (clientname, clientport))
    client[0].setblocking(1)

    while True:

        request = str(client[0].recv(1024))

        print(request)

        if len(request) < 4:
            continue;

        requestedServer = (request.split(' ')[1].split(':')[0],
                           int(request.split(' ')[1].split(':')[1]))
        if (requestedServer not in server[1] for server in currentServers):
            if request[2:9] == "CONNECT":
                server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server.connect(requestedServer)

                if(args.timeout > 0):
                    server.settimeout(args.timeout)
                else:
                    server.settimeout(None)

                currentServers.append((server, requestedServer))
            else:
                print("Error: Non-connect method on unconnected server")
                exit
        else:
            server = [i[0] for i in currentServers if i[1] is requestedServer]


        if threading.activeCount() <= args.numworkers:
            threadTemp = thread(client[0], server, request)
            threadTemp.start()







parser = argparse.ArgumentParser(description = "Proxy Server", add_help = False)
parser.add_argument('-h', '--help', action ="store_true", default = False)
parser.add_argument('-v', '--version', action ="store_true", default = False)
parser.add_argument('-p', '--port', type=int)
parser.add_argument('-n', '--numworkers', type=int, default = 10)
parser.add_argument('-t', '--timeout', type=int, default = -1)
parser.add_argument('-l', '--log', action ="store_true", default = False)

args = parser.parse_args()
main(args)
