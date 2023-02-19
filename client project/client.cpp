#include <sfml/Network.hpp>
#include <sfml/Graphics.hpp>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "lobby.h"
#include "game.h"
#include "Win.h"
#include "States.h"
#include "Player.h"

//setup states
States state = States::lobby;
Lobby lobby_state;
Game game_state;
Win win_state;

//other players data
Player players[4];
int player_count = 0;

//clients data
int ID;
std::string name;
std::string server_ip;
unsigned short server_tcp_port;
bool disconnect = false;

//sockets
sf::TcpSocket socket;
sf::UdpSocket udp;
unsigned short server_udp_port;


sf::Clock timer; //used to calc delta time
bool init = true; //keeps track of if states init function should be called
bool focused = true; //keeps track of if sfml window is in focus (only necessary when running multiple games on same pc)

//connects client to the server
bool ServerConnection()
{
	//set socket to blocking before trying to connect to server
	socket.setBlocking(false);

	server_ip = "127.0.0.1";
	server_tcp_port = 55001;

	//recieve name from user
	std::cout << "please enter name" << std::endl;
	std::cin >> name;

	////recieve server IP/port from user
	//std::cout << std::endl << "please enter server IP" << std::endl;
	//std::cin >> server_ip;
	//std::cout << std::endl << "please enter server port number" << std::endl;
	//std::cin >> server_tcp_port;

	//connect to server
	socket.connect(server_ip, server_tcp_port);

	//recieve ID from server
	sf::Packet player_data_packet;

	sf::SocketSelector selector;

	selector.add(socket);
	if (selector.wait(sf::seconds(3)))
	{
		if (socket.receive(player_data_packet) == sf::Socket::Done)
		{
			std::cout << std::endl << "connected" << std::endl;
		}
		else
		{
			std::cout << std::endl << "failed to connect" << std::endl;
			socket.disconnect();
			return false;
		}
	}
	//if you dont recieve anything back from server then connection failed
	else
	{
		std::cout << std::endl << "failed to connect" << std::endl;
		socket.disconnect();
		return false;
	}
	player_data_packet >> ID >> server_udp_port;

	//bind udp to any available port
	udp.bind(sf::Socket::AnyPort);

	//send name, ID and udp port to server
	sf::Packet p_name;
	PacketTypes type = PacketTypes::PlayerData;
	sf::IpAddress ip;
	p_name << static_cast<int>(type) << name << ID << ip.getLocalAddress().toString() << udp.getLocalPort();
	socket.send(p_name);
	
	//reset disconnect bool
	disconnect = false;

	//set socket to back to non-blocking 
	socket.setBlocking(false);
	return true;
}

int main()
{
	//set udp socket to non-blocking
	udp.setBlocking(false);

	//set player colours
	players[0].setFillColor(sf::Color::Red);
	players[1].setFillColor(sf::Color::Blue);
	players[2].setFillColor(sf::Color::Green);
	players[3].setFillColor(sf::Color::Magenta);

	//connect to server
	while (true)
	{
		if (ServerConnection())
		{
			break;
		}
	}

	//open sfml game window
	sf::RenderWindow window(sf::VideoMode(800, 500), name, sf::Style::Default);
	window.setFramerateLimit(60);
	while (true)
	{
		//update delta time
		float dt = timer.restart().asSeconds();

		//update window
		sf::Event Event;
		window.pollEvent(Event);
		switch (Event.type)
		{
		case sf::Event::Closed:
			window.close();
			break;
		case sf::Event::GainedFocus:
			focused = true;
			break;
		case sf::Event::LostFocus:
			focused = false;
			break;
		}

		//update and render current state 
		switch (state)
		{
		case States::lobby: 
			if (init)
			{
				lobby_state.init(init, players, socket, window);
			}
			lobby_state.update(name, socket, window, Event, state, ID, players, player_count, init, disconnect);
			lobby_state.draw(window, player_count, players);
			break;
		case States::game:
			if (init)
			{
				game_state.init(window, init, ID, player_count, udp, socket, players, server_ip, server_tcp_port, server_udp_port);
			}
			game_state.update(window, ID, player_count, udp, socket, dt, state, players, init, focused, Event, disconnect);
			game_state.draw(window, player_count, players);
			break;
		case States::win:
			if (init)
			{
				lobby_state.init(init, players, socket, window);
				win_state.init(init, players, player_count, window);
			}
			lobby_state.update(name, socket, window, Event, state, ID, players, player_count, init, disconnect);
			win_state.update(window, Event, state,players, player_count, init);
			win_state.draw(window, player_count, players);
			break;
		}
		if (disconnect)
		{
			//unbind udp socket
			udp.unbind();
			socket.disconnect();

			//close program
			return 0;
		}
	}
	return 0;
}