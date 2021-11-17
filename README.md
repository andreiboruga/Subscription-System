# Subscription-System
UDP client can be started with a command such as:
	- python3 udp_client.py <IP_SERVER> <SERVER_PORT>
	- python3 udp_client.py --mode manual <IP_SERVER> <SERVER_PORT>
	- python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json --mode random --delay 2000 <IP_SERVER> <SERVER_PORT>

TCP client can be started with:
	- ./subscriber <ID_CLIENT> <IP_SERVER> <SERVER_PORT>
where <ID_CLIENT> is a string of maximum 10 characters.

The server can be started with:
	- ./server <SERVER_PORT>

TCP client commands:
	"subscribe <TOPIC_NAME> <SF>"
where <SF> is the "store and forward flag" values = {0, 1}
	If SF is set to 1 then a client subscribed to a topic will have
his messages stored while he is not connected and when he connects
back again he will receive all of the messages that were sent when
he was offline.

	"unsubscribe <TOPIC_NAME>"
	"exit" -> disconnects from the server and closes

Server commands:
	"exit" -> closes itself and all the TCP clients
