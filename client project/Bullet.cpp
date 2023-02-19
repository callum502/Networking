#include "Bullet.h"

Bullet::Bullet(sf::Vector2f pos)
{
	this->setSize(sf::Vector2f(10, 10));
	this->setFillColor(sf::Color::White);
	this->setPosition(pos.x, pos.y);
}
