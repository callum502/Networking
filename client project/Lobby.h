#pragma once
#include <sfml/Network.hpp>
#include <sfml/Graphics.hpp>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include "States.h"
#include "Player.h"

class Lobby
{
private:

public:
	Lobby();
	void init(bool& init, Player players[4], sf::TcpSocket& socket, sf::RenderWindow& window);
	void update(std::string name, sf::TcpSocket& socket, sf::RenderWindow& window, sf::Event Event, States& state, int& ID, Player players[4], int& player_count, bool& init, bool& disconnect);
	void draw(sf::RenderWindow& window, int& player_count, Player players[4]);
	bool can_start;
	bool start;

	//text
	sf::Font font;
	sf::Text text_names[4];
	sf::Text text_ready[4];
	sf::Text text_start;
	sf::Text text_ready_up;

	//player texture
	sf::Texture pandaTexture;
};

