#pragma once
#include <sfml/Network.hpp>
#include <sfml/Graphics.hpp>
#include <iostream>
#include <string>
#include <string.h>

#include "States.h"
#include "Player.h"

class Win
{
public:
	Win();
	void init(bool& init, Player players[4], int player_count, sf::RenderWindow& window);
	void update(sf::RenderWindow& window, sf::Event Event, States& state, Player players[4], int player_count, bool& init);
	void draw(sf::RenderWindow& window, int player_count, Player players[4]);
	int winner_ID;

	//text
	sf::Font font;
	sf::Text winner_text;
	sf::Text enter_text;

	//Textures & Sprites
	sf::Texture confettiTexture;
	sf::Sprite confettiSprite;
	sf::Texture hatTexture;
	sf::Sprite hatSprite;
};

