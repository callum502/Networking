#include "Lobby.h"

Lobby::Lobby()
{
	//load font
	if (!font.loadFromFile("arial.ttf"))
	{
		std::cout << "failed to load font" << std::endl;
	}

	//setup text
	text_start.setFont(font);
	text_start.setString("All players are ready, press enter to start!!!");
	text_start.setCharacterSize(24);
	text_start.setStyle(sf::Text::Bold | sf::Text::Italic);
	text_start.setFillColor(sf::Color::Red);

	text_ready_up.setFont(font);
	text_ready_up.setString("Press spacebar to toggle ready up!");
	text_ready_up.setCharacterSize(24);
	text_ready_up.setStyle(sf::Text::Bold | sf::Text::Italic);
	text_ready_up.setFillColor(sf::Color::Red);

	for (int i = 0; i < 4; i++)
	{
		text_names[i].setFont(font);
		text_names[i].setString("Hello world");
		text_names[i].setCharacterSize(24);
		text_names[i].setStyle(sf::Text::Bold | sf::Text::Italic);

		text_ready[i].setFont(font);
		text_ready[i].setString("Hello world");
		text_ready[i].setCharacterSize(24);
		text_ready[i].setStyle(sf::Text::Bold);
	}
	text_names[0].setPosition(50,120);
	text_names[1].setPosition(250, 120);
	text_names[2].setPosition(450, 120);
	text_names[3].setPosition(650, 120);

	text_names[0].setFillColor(sf::Color::Red);
	text_names[1].setFillColor(sf::Color::Blue);
	text_names[2].setFillColor(sf::Color::Green);
	text_names[3].setFillColor(sf::Color::Magenta);

	text_ready[0].setPosition(50, 250);
	text_ready[1].setPosition(250, 250);
	text_ready[2].setPosition(450, 250);
	text_ready[3].setPosition(650, 250);

	text_ready[0].setFillColor(sf::Color::Red);
	text_ready[1].setFillColor(sf::Color::Blue);
	text_ready[2].setFillColor(sf::Color::Green);
	text_ready[3].setFillColor(sf::Color::Magenta);

}

void Lobby::init(bool& init, Player players[4], sf::TcpSocket& socket, sf::RenderWindow& window)
{
	init = false;

	//position text
	text_start.setPosition(window.getSize().x / 2 - text_ready_up.getGlobalBounds().width / 2, window.getSize().y * 0.8);
	text_ready_up.setPosition(window.getSize().x / 2 - text_ready_up.getGlobalBounds().width / 2, window.getSize().y * 0.7);

	//load player texture
	if (!pandaTexture.loadFromFile("panda.png"))
	{
		std::cout << "error loading background texture" << std::endl;
	}

	//set player textures
	players[0].setTexture(&pandaTexture);
	players[1].setTexture(&pandaTexture);
	players[2].setTexture(&pandaTexture);
	players[3].setTexture(&pandaTexture);

	//set player size, pos, ready status
	players[0].setSize(sf::Vector2f(100, 100));
	players[0].setPosition(50, 150);
	players[0].ready = false;

	players[1].setSize(sf::Vector2f(100, 100));
	players[1].setPosition(250, 150);
	players[1].ready = false;

	players[2].setSize(sf::Vector2f(100, 100));
	players[2].setPosition(450, 150);
	players[2].ready = false;

	players[3].setSize(sf::Vector2f(100, 100));
	players[3].setPosition(650, 150);
	players[3].ready = false;

	//reset bools
	can_start = false;
	start = false;

	//reset ready text
	text_ready[0].setString("Not Ready");
	text_ready[1].setString("Not Ready");
	text_ready[2].setString("Not Ready");
	text_ready[3].setString("Not Ready");

}

void Lobby::update(std::string name, sf::TcpSocket& socket, sf::RenderWindow& window, sf::Event Event, States& state, int& ID, Player players[4], int& player_count, bool& init, bool& disconnect)
{
	if (Event.type == sf::Event::KeyPressed)
	{
		//toggles readyup if spacebar is pressed
		if (Event.key.code == sf::Keyboard::Space and state == States::lobby)//to prevent toggling ready from win screen
		{
			sf::Packet packet;
			PacketTypes type = PacketTypes::ReadyToggle;
			//toggle client side ready status
			players[ID - 1].ready = !players[ID - 1].ready;
			//send updated ready status to server for other clients to recieve
			packet << static_cast<int>(type) << ID << players[ID - 1].ready << start;
			socket.send(packet);
		}
		//send start to server if enter is pressed and all players are ready
		else if (Event.key.code == sf::Keyboard::Enter && can_start)
		{
			sf::Packet p_start;
			PacketTypes type = PacketTypes::ReadyToggle;
			//set client side start to true
			start = true;
			//send start to server so other clients know to start 
			p_start << static_cast<int>(type) << ID << players[ID - 1].ready << start;
			socket.send(p_start);
		}
		//disconnect from server if esc is pressed
		else if (Event.key.code == sf::Keyboard::Key::Escape)
		{
			socket.disconnect();
			window.close();
			disconnect = true;
		}
	}

	//recieve packet from server
	sf::Packet lobby_data;

	//if successfully recieved
	if (socket.receive(lobby_data) == sf::Socket::Done)
	{
		//get type of packet //0 is updated ID //1 is updated lobby data
		int packet_type;
		lobby_data >> packet_type;

		//update lobby data
		if (packet_type == 0)
		{
			lobby_data >> ID;
		}
		//update and display local lobby data
		else if (packet_type ==1)
		{
			lobby_data >> player_count;
			for (int i = 0; i < player_count; i++)
			{
				//extract updated player data
				lobby_data >> players[i].name;
				lobby_data >> players[i].ready;
				lobby_data >> players[i].ID;
				//update names
				text_names[i].setString(players[i].name);

				//update ready up text
				if (players[i].ready)
				{
					text_ready[players[i].ID - 1].setString("Ready");
				}
				else
				{
					text_ready[players[i].ID - 1].setString("Not Ready");
				}
			}

			//recieve and update local can_start bool (used to determine if we should draw "enter to start" text)
			lobby_data >> can_start;
		}
		//if start packet recieved, change game state
		lobby_data >> start;
		if (start)
		{
			state = States::game;
			init = true;
			start = false;
		}
	}
}

void Lobby::draw(sf::RenderWindow& window, int& player_count, Player players[4])
{
	window.clear();
	//draw players, names and text
	for (int i=0; i<player_count; i++)
	{
		window.draw(text_ready[i]);
		window.draw(players[i]);
		window.draw(text_names[i]);
		window.draw(text_ready_up);
		if (can_start)
		{
			window.draw(text_start);
		}
	}
	window.display();
}
