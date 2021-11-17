#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define MAX_WAITING_CLIENTS 5


struct topic
{
	char topic_name[50];
	int *sub_clients;
	int *sf_flags;
	int max_clients;
	int curr_clients;
};

struct CMD
{
	char action[15];
	char topic[50];
	int sf;
};

struct client
{

	char id[10];
	int curr_sockfd;
	struct udp_to_tcp *new_messages;
	int max_messages;
	int curr_messages;

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

void remove_from_vector(int *v, int n, int indice)
{
	int j;
	for(j = indice; j < n - 1; j++)
		v[j] = v[j+1];
}


int main(int argc, char *argv[])
{

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	struct topic *topics = (struct topic *)calloc(100, sizeof(struct topic));
	int max_topics = 100;
	int topics_count = 0;


	struct client *clients = (struct client *)calloc(100, sizeof(struct client));
	int max_clients = 100;
	int clients_count = 0;

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax, ret;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	if(argv[1] == NULL)
	{
		printf("Error.\nUsage: ./server <PORT_NUMBER>\n");
		return 1;
	}
	int port = strtol(argv[1], NULL, 10);
	if(port == 0)
	{
		printf("Error.\nUsage: ./server <PORT_NUMBER>\n");
		return 1;
	}
	int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_sock < 0)
	{
		perror("Eroare creare socket udp.");
		return 1;
	}

	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_sock < 0)
	{
		perror("Eroare creare socket tcp.");
		return 1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	int b = bind(udp_sock, (struct sockaddr*) &addr, sizeof(addr));
	if(b < 0)
	{
		perror("asocierea intre socket udp si adresa.");
		close(udp_sock);
		return 1;
	}

	ret = bind(tcp_sock, (struct sockaddr *) &addr, sizeof(addr));
	if(ret < 0)
	{
		perror("asocierea intre socket tcp si adresa.");
		close(tcp_sock);
		return 1;
	}

	ret = listen(tcp_sock, MAX_WAITING_CLIENTS);
	if(ret < 0)
	{
		perror("listen");
		close(tcp_sock);
		return 1;
	}


	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(udp_sock, &read_fds);
	FD_SET(tcp_sock, &read_fds);
	if(udp_sock > tcp_sock)
		fdmax = udp_sock;
	else fdmax = tcp_sock;

	while(1) 
	{
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if(ret < 0)
		{
			perror("select");
			close(udp_sock);
			return 1;
		}

		for(int i = 0; i <= fdmax; i++)
		{

			if (FD_ISSET(i, &tmp_fds))
			{

				if(i == STDIN_FILENO)
				{
					char *s = (char *)malloc(sizeof(char) * 100);
					scanf("%s", s);
					if(strcmp(s, "exit") == 0)
					{
						free(s);
						goto exit;
					}
					free(s);
				}
				else if(i == udp_sock)
				{

					struct udp_msg mesaj_udp;
					struct sockaddr_in cli_addr;
					socklen_t socklen = sizeof(cli_addr);

					ret = recvfrom(udp_sock, &mesaj_udp, sizeof(struct udp_msg), 0, (struct sockaddr*) &cli_addr, &socklen);
					if(ret < 0)
					{
						perror("Eroare primire mesaj udp");
						goto exit;
					}

					


					for(int j = 0; j < topics_count; j++)
					{
						if(strcmp(mesaj_udp.topic, topics[j].topic_name) == 0)
						{

							struct udp_to_tcp msg;
							strcpy(msg.ip_udp, inet_ntoa(cli_addr.sin_addr));
							msg.port_udp = ntohs(cli_addr.sin_port);
							msg.mesaj_udp = mesaj_udp;

							for(int k = 0; k < topics[j].curr_clients; k++)
							{

								if(clients[topics[j].sub_clients[k]].curr_sockfd == -1)
								{

									if(topics[j].sf_flags[k] == 1)
									{

										if(clients[topics[j].sub_clients[k]].new_messages == NULL)
										{
											clients[topics[j].sub_clients[k]].new_messages = (struct udp_to_tcp *)calloc(10, sizeof(struct udp_to_tcp));
											clients[topics[j].sub_clients[k]].max_messages = 10;
											clients[topics[j].sub_clients[k]].curr_messages = 0;
										}

										memcpy(&clients[topics[j].sub_clients[k]].new_messages[clients[topics[j].sub_clients[k]].curr_messages], &msg, sizeof(struct udp_to_tcp));
										clients[topics[j].sub_clients[k]].curr_messages++;

										if(clients[topics[j].sub_clients[k]].curr_messages == clients[topics[j].sub_clients[k]].max_messages)
										{
											clients[topics[j].sub_clients[k]].max_messages *= 2;
											clients[topics[j].sub_clients[k]].new_messages = (struct udp_to_tcp *)realloc(clients[topics[j].sub_clients[k]].new_messages,
																									 clients[topics[j].sub_clients[k]].max_messages);
										}

									}


								}
								else
								{
									ret = send(clients[topics[j].sub_clients[k]].curr_sockfd, &msg, sizeof(struct udp_to_tcp), 0);
									if(ret < 0)
									{
										perror("send udp to client");
										goto exit;
									}

								}

							}
							break;
						}
					}

				}
				else if(i == tcp_sock)
				{
					int j, id_error = 0;
					char tmp_id[10];
					struct sockaddr_in cli_addr;
					socklen_t socklen = sizeof(cli_addr);

					int newsock = accept(tcp_sock, (struct sockaddr *) &cli_addr, &socklen);
					if(newsock < 0)
					{
						perror("accept");
						goto exit;
					}

					FD_SET(newsock, &read_fds);



					///////////////////////////NEAGLE DISABLE
					int flag = 1;
    				ret = setsockopt(newsock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int)); 
   					if (ret < 0)
  					{
    					perror("Neagle");
    					goto exit;
  					}


					///////////////////////////NEAGLE DISABLE





					ret = recv(newsock, &tmp_id, 10, 0);
					if(ret < 0)
					{
						perror("reading id");
						goto exit;
					}

					for(j = 0; j < clients_count; j++)
						if(strcmp(clients[j].id, tmp_id) == 0)
						{
							if(clients[j].curr_sockfd == -1)
								clients[j].curr_sockfd = newsock;
							else
								id_error = 1;
							break;
						}

					if(j == clients_count)
					{
						strcpy(clients[clients_count].id, tmp_id);
						clients[clients_count].curr_sockfd = newsock;
						clients_count++;
						if(clients_count == max_clients)
						{
							max_clients *= 2;
							clients = (struct client *)realloc(clients, max_clients);
						}
					}

					ret = send(newsock, &id_error, sizeof(id_error), 0);
					if(ret < 0)
					{
						perror("send id error");
						goto exit;
					}

					if(id_error == 1)
					{
						printf("Client %s already connected.\n", tmp_id);
						close(newsock);
						FD_CLR(newsock, &read_fds);
						continue;
					}

					if (newsock > fdmax) { 
						fdmax = newsock;
					}

					printf("New client %s connected from %s:%d.\n",
							tmp_id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

					for(int k = 0; k < clients[j].curr_messages; k++)
					{
						ret = send(newsock, &clients[j].new_messages[k], sizeof(struct udp_to_tcp), 0);
						if(ret < 0)
						{
							perror("send new messages");
							free(clients[j].new_messages);
							goto exit;
						}
					}

					if(clients[j].new_messages != NULL)
					{
						clients[j].max_messages = 0;
						clients[j].curr_messages = 0;
						free(clients[j].new_messages);
					}

				}
				else
				{

					struct CMD *command = (struct CMD *)malloc(sizeof(struct CMD));

					ret = recv(i, command, sizeof(struct CMD), 0);
					if(ret < 0)
					{
						perror("recv");
						free(command);
						goto exit;
					}

					if(ret == 0)
					{
						char tmp_id[10];

						for(int j = 0; j <= clients_count; j++)
						{
							if(clients[j].curr_sockfd == i)
							{
								strcpy(tmp_id, clients[j].id);
								clients[j].curr_sockfd = -1;
								break;
							}
						}

						printf("Client %s disconnected.\n", tmp_id);


						close(i);

						FD_CLR(i, &read_fds);
						free(command);
					}
					else
					{

						int client_number;
						for(int j = 0; j < clients_count; j++)
						{
							if(clients[j].curr_sockfd == i)
							{
								client_number = j;
								break;
							}
						}
						
						if(strcmp(command->action, "subscribe") == 0)
						{
							int topic_exists = 0;
							int already_subbed = 0;


							for(int j = 0; j < topics_count; j++)
							{
								if(strcmp(topics[j].topic_name, command->topic) == 0)
								{
									topic_exists = 1;

									for(int k = 0; k < topics[j].curr_clients; k++)
									{
										if(topics[j].sub_clients[k] == client_number)
										{
											already_subbed = 1;
											break;
										}
									}

									if(already_subbed == 0)
									{
										topics[j].sub_clients[topics[j].curr_clients] = client_number;
										topics[j].sf_flags[topics[j].curr_clients] = command->sf;
										topics[j].curr_clients++;
										if(topics[j].curr_clients == topics[j].max_clients)
										{
											topics[j].max_clients *= 2;
											topics[j].sub_clients = (int *)realloc(topics[j].sub_clients, topics[j].max_clients);
											topics[j].sf_flags = (int *)realloc(topics[j].sf_flags, topics[j].max_clients);
										}
									}

									break;
								}
							}

							if(topic_exists == 0)
							{
								strcpy(topics[topics_count].topic_name, command->topic);
								topics[topics_count].sub_clients = (int *)calloc(100, sizeof(int));
								topics[topics_count].sf_flags = (int *)calloc(100, sizeof(int));
								topics[topics_count].max_clients = 100;
								topics[topics_count].curr_clients = 1;
								topics[topics_count].sub_clients[0] = client_number;
								topics[topics_count].sf_flags[0] = command->sf;
								topics_count++;
								if(topics_count == max_topics)
								{
									max_topics *= 2;
									topics = (struct topic *)realloc(topics, max_topics);
								}

							}

						}
						else
						{
							for(int j = 0; j < topics_count; j++)
							{
								if(strcmp(topics[j].topic_name, command->topic) == 0)
								{
									for(int k = 0; k < topics[j].curr_clients; j++)
									{
										if(topics[j].sub_clients[k] == client_number)
										{
											remove_from_vector(topics[j].sub_clients, topics[j].curr_clients, k);
											remove_from_vector(topics[j].sf_flags, topics[j].curr_clients, k);
											topics[j].curr_clients--;
											break;
										}
									}
									break;
								}
							}
						}

						free(command);
						
					}

				}

			}

		}




	}




	exit:


	for(int j = 0; j < max_topics; j++)
	{
		if(topics[j].sub_clients != NULL)
		{
			free(topics[j].sub_clients);
			free(topics[j].sf_flags);
		}
		else
			break;
	}
	free(topics);
	free(clients);
	close(udp_sock);
	close(tcp_sock);
	return 0;
}