#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define CMD_BUFF_LEN 100

struct CMD
{
	char action[15];
	char topic[50];
	int sf;
};

struct udp_msg
{
	char topic[50];
	char tip;
	char content[1500];
};

struct udp_to_tcp
{
	char ip_udp[20];
	int port_udp;
	struct udp_msg mesaj_udp;

};


int main(int argc, char *argv[])
{

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
	int id_error;


	int sock, ret, port;
	char id[10];

	char *buffer = (char *)malloc(sizeof(char) * 100); ///////////////////////////////////
	size_t buffsize = 100;  /////////////////////////////
	struct sockaddr_in serv_addr;

	if(argc < 3)
	{
		printf("Error.\nUsage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n");
		free(buffer);
		return 1;
	}

	port = atoi(argv[3]);
	strncpy(id, argv[1], 10);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		perror("Eroare creare socket tcp.");
		free(buffer);
		return 1;
	}




///////////////////////////NEAGLE DISABLE
	int flag = 1;
    int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int)); 
    if (result < 0)
    {
    	perror("Neagle");
    	free(buffer);
    	return 1;
    }


///////////////////////////NEAGLE DISABLE







	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	if(ret == 0)
	{
		perror("inet_aton");
		free(buffer);
		return 1;
	}

	ret = connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	if(ret < 0)
	{
		perror("connect");
		free(buffer);
		return 1;
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sock, &read_fds);
	fdmax = sock;

	ret = send(sock, &id, 10, 0);
	if(ret < 0)
	{
		perror("send id");
		free(buffer);
		close(sock);
		return 1;
	}

	ret = recv(sock, &id_error, sizeof(id_error), 0);
	if(ret < 0)
	{
		perror("recieving id_error value");
		free(buffer);
		close(sock);
		return 1;
	}

	if(id_error == 1)
	{
		free(buffer);
		close(sock);
		return 1;
	}

	while(1)
	{

		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if(ret < 0)
		{
			perror("select");
			free(buffer);
			close(sock);
			return 1;
		}

		for(int i = 0; i <= fdmax; i++)
		{

			if (FD_ISSET(i, &tmp_fds))
			{

				if(i == STDIN_FILENO)
				{

					struct CMD *command;

					memset(buffer, 0, CMD_BUFF_LEN);
					getline(&buffer, &buffsize, stdin);
					buffer[strlen(buffer) - 1] = '\0';
					if(strcmp(buffer, "exit") == 0)
					{
						goto exit;
					}
					else if(strncmp(buffer, "subscribe", 9) == 0)
					{
						command = (struct CMD *)malloc(sizeof(struct CMD));
						sscanf(buffer, "%s %s %d", command->action, command->topic, &command->sf);
						printf("Subscribed to topic.\n");
					}
					else if(strncmp(buffer, "unsubscribe", 11) == 0)
					{
						command = (struct CMD *)malloc(sizeof(struct CMD));
						sscanf(buffer, "%s %s", command->action, command->topic);
						command->sf = -1;
						printf("Unsubscribed from topic.\n");
					}
					else
						continue;


					ret = send(sock, command, sizeof(struct CMD), 0);
					if(ret < 0)
					{
						perror("send command");
						free(command);
						free(buffer);
						close(sock);
						return 1;
					}

					free(command);

				}
				else
				{

					struct udp_to_tcp msg;

					ret = recv(i, &msg, sizeof(struct udp_to_tcp), 0);
					if(ret < 0)
					{
						perror("recv");
						free(buffer);
						close(sock);
						return 1;
					}
					if(ret == 0)
					{
						goto exit;
					}

					if(msg.mesaj_udp.tip == 0)
					{
						printf("%s:%d - %s - INT - ", msg.ip_udp, msg.port_udp, msg.mesaj_udp.topic);
						char c = msg.mesaj_udp.content[0];
						uint32_t number;
						
						memcpy(&number, msg.mesaj_udp.content+1, sizeof(uint32_t));
						number = ntohl(number);

						if(c == 0)
						{
							printf("%u\n", number);
						}
						else
						{
							printf("-%u\n", number);
						}
					}
					else if(msg.mesaj_udp.tip == 1)
					{
						printf("%s:%d - %s - SHORT_REAL - ", msg.ip_udp, msg.port_udp, msg.mesaj_udp.topic);
						uint16_t number;
						memcpy(&number, msg.mesaj_udp.content, sizeof(uint16_t));
						number = ntohs(number);
						float f = number/(100 * 1.0);
						printf("%.2f\n", f);
					}
					else if(msg.mesaj_udp.tip == 2)
					{
						printf("%s:%d - %s - FLOAT - ", msg.ip_udp, msg.port_udp, msg.mesaj_udp.topic);
						char c = msg.mesaj_udp.content[0];
						uint32_t number;
						memcpy(&number, msg.mesaj_udp.content+1, sizeof(uint32_t));
						number = ntohl(number);
						
						int power = 1;
						for(int k = 1; k <= (uint32_t)msg.mesaj_udp.content[5]; k++)
						{
							power *= 10;
						}
						float f = number/(power * 1.0);
						if(c == 0)
						{
							printf("%f\n", f);
						}
						else
						{
							printf("-%f\n", f);
						}

					}
					else if(msg.mesaj_udp.tip == 3)
					{
						printf("%s:%d - %s - STRING - %s\n", msg.ip_udp, msg.port_udp, msg.mesaj_udp.topic, msg.mesaj_udp.content);
					}



				}

			}
		}

	}
















	exit:

	free(buffer);
	close(sock);
	return 0;
}