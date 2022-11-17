TEMPLATE = subdirs
SUBDIRS = client server ostfile ostfilegui ostproto ostprotogui rpc extra

client.target = client
client.file = client/ostinato.pro
client.depends = ostfile ostfilegui ostproto ostprotogui rpc extra

server.target = server
server.file = server/drone.pro
server.depends = ostproto rpc

ostfile.file = common/ostfile.pro
ostfile.depends = ostproto

ostfilegui.file = common/ostfilegui.pro
ostfilegui.depends = ostfile

ostproto.file = common/ostproto.pro

ostprotogui.file = common/ostprotogui.pro
ostprotogui.depends = extra

rpc.file = rpc/pbrpc.pro
