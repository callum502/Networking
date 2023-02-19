#pragma once
#include <sfml/Network.hpp>
#include <sfml/Graphics.hpp>
#include <iostream>
#include <string>
#include <string.h>

struct Pos_Data {
	float x = 0;
	float y = 0;
	float time = 0;
};

class Player : public sf::RectangleShape
{
public:
	int ID;
	std::string name;
	bool ready;

	sf::Vector2f speed;
	bool alive;
	sf::Vector2f spawnpoint;
	Pos_Data pos_data[3];
};

