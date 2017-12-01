
import argparse
import socket
import threading
import http.server


class thread(threading.Thread):
    def __init__(self, client, server, request):
        threading.Thread.__init__(self)
        self.client = client
        self.server = server
        self.request = request
        pass
    def run(self):

        self.server.send(self.request)

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


    client = clientSocket.accept()
    client[0].setblocking(1)

    i = 10
    while i > 0:
        i -= 1

        request = client[0].recv(1024)


        print(request)



        requestedServer = (str(request).split(' ')[1].split(':')[0],
                           int(str(request).split(' ')[1].split(':')[1]))
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
