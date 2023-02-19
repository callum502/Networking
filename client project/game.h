#pragma once
#include <sfml/Network.hpp>
#include <sfml/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include "Bullet.h"
#include "States.h"
#include "Player.h"

struct Bullet_Data {
	float xpos = 0;
	float ypos = 0;
	float xspeed = 0;
	float yspeed = 0;
};

class Game
{
public:
	void init(sf::RenderWindow& window, bool& init, int ID, int& player_count, sf::UdpSocket& udp, sf::TcpSocket& tcp, Player players[4], std::string ip, unsigned short tcp_port, unsigned short udp_port);
	Game();
	void update(sf::RenderWindow& window, int ID, int& player_count, sf::UdpSocket& udp, sf::TcpSocket& tcp, float dt, States& state, Player players[4], bool& init, bool focused, sf::Event Event, bool& disconnect);
	void draw(sf::RenderWindow& window, int& player_count, Player players[4]);
private:
	sf::SocketSelector gameSelector;
	std::string server_ip;
	unsigned short server_tcp_port;
	unsigned short server_udp_port;

	//cooldowns
	float packet_cooldown;
	float shoot_cooldown;

	//bullet vectors
	std::vector<Bullet> bullets;
	std::vector<Bullet> enemy_bullets;

	//timer
	sf::Clock latency_timer;
	float latency = 0;
	float synced_timer;

	//text to display timer
	sf::Font font;
	sf::Text text_time;

	//predicted position based on 2 most recent packet positions and times
	float predictedX;
	float predictedY;

	//background
	sf::Texture backgroundTexture;
	sf::Sprite backgroundSprite;

	//music
	sf::Music music;
};

