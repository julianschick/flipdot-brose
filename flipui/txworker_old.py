import threading
import socket
import time
import sys
from queue import Queue, Empty

IP = "flipdot1"
PORT = 3000
BUFFER_SIZE = 64

STOP = 0x82

class TxWorker(threading.Thread):

    def __init__(self, led_queue, pixel_queue):
        super(TxWorker, self).__init__()
        self.led_queue = led_queue
        self.pixel_queue = pixel_queue
        self.sock = None
        self.connection_open = False
        self.last_tx = 0

    def get_from_queue(self, queue):
        items = []
        while not queue.empty() and len(items) < 10:
            try:
                items.append(queue.get(block=False))
            except Empty:
                break
        return items

    def run(self):
        while True:

            items = self.get_from_queue(self.led_queue)
            ack_counter = 0

            if self.connection_open and (time.time() - self.last_tx) > 2.5:
                self.sock.close()
                self.connection_open = False
                print("closed (no more tx)")

            if not self.connection_open:
                time.sleep(0.1)

            if items:
                try:
                    to_send = [0xA0]
                    for i, rgb in enumerate(items):
                        to_send.extend([rgb[0], rgb[1], rgb[2], 0x82 if i == len(items)-1 else 0x84])

                    if not self.connection_open:
                        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        self.sock.settimeout(0.5)
                        self.sock.connect((IP, PORT))
                        print("opened")
                        self.connection_open = True

                    self.sock.send(bytes(to_send))
                    self.last_tx = time.time()

                    #data = self.sock.recv(len(items)*2)
                    #for b in data:
                    #    if b == 0x80:
                    #        ack_counter += 1

                    #if ack_counter < len(items):
                    #    data = self.sock.recv(16)

                    #print(str(len(items)) + " led at once")
                except ConnectionRefusedError:
                    print("connection refused")
                except socket.timeout:
                    print(sys.exc_info()[0])
                    print("led tx error")
                    if self.sock and self.connection_open:
                        self.sock.close()
                        print("closed (err)")
                    self.sock = None
                    self.connection_open = False

            items = self.get_from_queue(self.pixel_queue)
            ack_counter = 0

            if items:
                try:
                    to_send = [0x94]

                    for i, px in enumerate(items):
                        to_send.extend([0x01 if px[2] else 0x00, px[0], px[1]])
                    to_send.append(STOP)

                    if not self.connection_open:
                        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        self.sock.settimeout(0.5)
                        self.sock.connect((IP, PORT))
                        print("opened")
                        self.connection_open = True

                    self.sock.send(bytes(to_send))
                    self.last_tx = time.time()

                    #data = self.sock.recv(len(items)*2)
                    #for b in data:
                    #    if b == 0x80:
                    #        ack_counter += 1

                    #if ack_counter < len(items):
                    #    data = self.sock.recv(16)

                    #print(str(len(items)) + " px at once")

                except ConnectionRefusedError:
                    print("connection refused")
                except socket.timeout:
                    print(sys.exc_info()[0])
                    print("px tx error")
                    if self.sock and self.connection_open:
                        self.sock.close()
                        print("closed (err)")
                    self.sock = None
                    self.connection_open = False


            # items = self.get_from_queue(self.pixel_queue)
            # ack_counter = 0
            #
            # if items:
            #     try:
            #         self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #         self.sock.settimeout(0.5)
            #         self.sock.connect((IP, PORT))
            #         self.sock.send(bytes([0x94, 0x01]))
            #
            #         for i, px in enumerate(items):
            #             self.sock.send(bytes([0x01 if px[2] else 0x00, px[0], px[1]]))
            #
            #         data = self.sock.recv(len(items)*2)
            #         for b in data:
            #             if b == 0x80:
            #                 ack_counter += 1
            #
            #         if ack_counter < len(items):
            #             data = self.sock.recv(16)
            #
            #         self.sock.send(bytes([0xD0]))
            #         print(str(len(items)) + " px at once")
            #     except:
            #         print("px tx error")
            #     finally:
            #         self.sock.close()

