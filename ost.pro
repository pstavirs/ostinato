TEMPLATE = subdirs
SUBDIRS = client server ostproto ostprotogui rpc binding extra

client.target = client
client.file = client/ostinato.pro
client.depends = ostproto ostprotogui rpc extra

server.target = server
server.file = server/drone.pro
server.depends = ostproto rpc

ostproto.file = common/ostproto.pro

ostprotogui.file = common/ostprotogui.pro
ostprotogui.depends = extra

rpc.file = rpc/pbrpc.pro

binding.target = binding
binding.depends = ostproto
