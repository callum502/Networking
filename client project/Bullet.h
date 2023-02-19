#pragma once
#include <sfml/Network.hpp>
#include <sfml/Graphics.hpp>
class Bullet : public sf::RectangleShape
{
public:
	Bullet(sf::Vector2f pos);
	sf::Vector2f speed;
};

