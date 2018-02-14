
import os
import psutil
import paramiko
from contextlib import contextmanager
from paramiko import client
from paramiko import SSHClient
import subprocess

class ssh:
    client = None
 
    def __init__(self, address, username, password):
        print("Connecting to server.")
        self.client = client.SSHClient()
        self.client.set_missing_host_key_policy(client.AutoAddPolicy())
        self.client.connect(address, int("22"),username, password, look_for_keys=False)
 
    def sendCommand(self, command):
        if(self.client):
            stdin, stdout, stderr = self.client.exec_command(command)
            while not stdout.channel.exit_status_ready():
                # Print data when available
                if stdout.channel.recv_ready():
                    alldata = stdout.channel.recv(1024)
                    prevdata = b"1"
                    while prevdata:
                        prevdata = stdout.channel.recv(1024)
                        alldata += prevdata
 
                    print(str(alldata))
        else:
            print("Connection not opened.")          

connection = ssh("oasys-lab.isi.edu", "vr", 'vr1234')

connection.sendCommand('python Memory.py')
