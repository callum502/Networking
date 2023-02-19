#include "game.h"

void Game::init(sf::RenderWindow& window, bool& init, int ID, int& player_count, sf::UdpSocket& udp, sf::TcpSocket& tcp, Player players[4], std::string ip, unsigned short tcp_port, unsigned short udp_port)
{
	init = false;

	server_ip = ip;
	server_tcp_port = tcp_port;
	server_udp_port = udp_port;

	//reset socket selector
	gameSelector.clear();
	gameSelector.add(udp);
	gameSelector.add(tcp);

	//reset bullet vectors
	bullets.clear();
	enemy_bullets.clear();

	//setup background
	backgroundSprite.setTexture(backgroundTexture);
	backgroundSprite.setScale(sf::Vector2f(2, 2));

	//play music
	music.setVolume(25);
	music.play();

	//reset cooldowns
	shoot_cooldown = 0;
	packet_cooldown = 0;
	synced_timer = 0;

	//set spawnpoints
	players[0].spawnpoint = sf::Vector2f(players[1].getSize().x, players[1].getSize().y); // top left
	players[1].spawnpoint = sf::Vector2f(window.getSize().x - players[1].getSize().x * 2, players[1].getSize().y); //top right
	players[2].spawnpoint = sf::Vector2f(players[1].getSize().x, window.getSize().y - players[1].getSize().y * 2); //bottom left
	players[3].spawnpoint = sf::Vector2f(window.getSize().x - players[1].getSize().x * 2, window.getSize().y - players[1].getSize().y * 2); //bottom right

	//reset players pos, size etc
	for (int i = 0; i < player_count; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			players[i].pos_data[j].time = 0;
			players[i].pos_data[j].x = players[i].spawnpoint.x;
			players[i].pos_data[j].y = players[i].spawnpoint.y;
		}
		players[i].alive = true;
		players[i].setPosition(players[i].spawnpoint);
		players[i].setSize(sf::Vector2f(50, 50));
	}

	//Get synced timer
	sf::Packet timer;
	PacketTypes type = PacketTypes::TimerSync;
	timer << static_cast<int>(type);
	//start latency timer
	latency_timer.restart();
	//send to server (recieved back from server in update function)
	tcp.send(timer);
}

Game::Game()
{
	//load font
	if (!font.loadFromFile("arial.ttf"))
	{
		std::cout << "error loading font" << std::endl;
	}

	//setup timer text
	text_time.setFont(font);
	text_time.setString("All players are ready, press enter to start!!!");
	text_time.setCharacterSize(24);
	text_time.setStyle(sf::Text::Bold | sf::Text::Italic);
	text_time.setPosition(50, 500);
	text_time.setFillColor(sf::Color::Red);
	
	//load background
	if (!backgroundTexture.loadFromFile("bg2.jpg"))
	{
		std::cout << "error loading background texture" << std::endl;
	}

	//load/play music
	if (!music.openFromFile("music.ogg"))
	{
		std::cout << "error loading music" << std::endl;
	}
	music.setLoop(true);
}

void Game::update(sf::RenderWindow& window, int ID, int& player_count, sf::UdpSocket& udp, sf::TcpSocket& tcp, float dt, States& state, Player players[4], bool& init, bool focused, sf::Event Event, bool& disconnect)
{
	//increase timer/cooldowns
	synced_timer += dt;
	shoot_cooldown -= dt;
	packet_cooldown -= dt;

	//if socket is ready then recieve packet otherwise move on 
	if (gameSelector.wait(sf::microseconds(1))) //zero acts as infinite so we have to wait 1 microsecond
	{
		sf::Packet packet;
		sf::IpAddress sender;
		unsigned short port;
		//if udp socket is ready then recieve packet (sender/port are not used as we should know the servers IP/Port already)
		if (udp.receive(packet, sender, port) == sf::Socket::Done)
		{
			int recID = 0;
			//get ID from packet
			packet >> recID;

			//extract position data from packet
			float rec_time;
			float rec_xpos;
			float rec_ypos;

			packet >> rec_time >> rec_xpos >> rec_ypos;

			//in case packets arrive in wrong order
			if (rec_time > players[recID - 1].pos_data[0].time)
			{
				//bump pos data up in array
				players[recID - 1].pos_data[2].time = players[recID - 1].pos_data[1].time;
				players[recID - 1].pos_data[2].x = players[recID - 1].pos_data[1].x;
				players[recID - 1].pos_data[2].y = players[recID - 1].pos_data[1].y;

				players[recID - 1].pos_data[1].time = players[recID - 1].pos_data[0].time;
				players[recID - 1].pos_data[1].x = players[recID - 1].pos_data[0].x;
				players[recID - 1].pos_data[1].y = players[recID - 1].pos_data[0].y;

				//update most recent packet data for the receieved ID
				players[recID - 1].pos_data[0].time = rec_time;
				players[recID - 1].pos_data[0].x = rec_xpos;
				players[recID - 1].pos_data[0].y = rec_ypos;
			}
		}
		//if udp socket is not ready then test the tcp socket
		if (tcp.receive(packet) == sf::Socket::Done)
		{
			//get packet type
			int packet_type = -1;
			packet >> packet_type;

			//if packet type is timer sync then resync the timer
			if (static_cast<PacketTypes>(packet_type) == PacketTypes::TimerSync)
			{
				//calculate synced time
				packet >> synced_timer;
				synced_timer -= latency; //minus old latency
				latency = latency_timer.getElapsedTime().asSeconds() / 2; //calc new latency
				synced_timer += latency; //add new latency
			}
			//if packet type is a bullet collision
			else if (static_cast<PacketTypes>(packet_type) == PacketTypes::BulletCollision)
			{
				//recieve ID of player and new alive status
				int rec_id;
				bool rec_alive;
				packet >> rec_id >> rec_alive;
				//set player to dead
				players[rec_id - 1].alive = rec_alive;

				//calculate how many players are still alive
				int num_alive_players = 0;
				for (int i = 0; i < player_count; i++)
				{
					if (players[i].alive)
					{
						num_alive_players++;
					}
				}

				//if there is only one player remaining, change to win state
				if (num_alive_players < 2)
				{
					state = States::win;
					init = true;
					music.stop();
					return;
				}
			}
			//if packet type is a bullet that has been fired
			else if (static_cast<PacketTypes>(packet_type) == PacketTypes::BulletFired)
			{
				//extract player Id, bullet pos and bullet speed
				sf::Vector2f bullet_pos;
				sf::Vector2f bullet_speed;
				int rec_ID = -1;
				float time_sent;
				packet >> rec_ID >> bullet_pos.x >> bullet_pos.y >> bullet_speed.x >> bullet_speed.y >> time_sent;

				//calculate where bullet should now that it has been sent to server and recieved by client (add speed * change in time)
				float frames = (synced_timer - time_sent) / dt;
				float synced_posx = bullet_pos.x + bullet_speed.x  * (synced_timer - time_sent);
				float synced_posy = bullet_pos.y + bullet_speed.y * (synced_timer - time_sent);

				//add to enemy vector
				Bullet bullet(sf::Vector2f(synced_posx, synced_posy));
				//set bullet colour based on which player fired it
				switch (rec_ID)
				{
				case 1:
					bullet.setFillColor(sf::Color::Red);
					break;
				case 2:
					bullet.setFillColor(sf::Color::Blue);
						break;
				case 3:
					bullet.setFillColor(sf::Color::Green);
					break;
				case 4:
					bullet.setFillColor(sf::Color::Magenta);
					break;
				default:
					break;
				}

				//set speed and add bullet to vector
				bullet.speed = sf::Vector2f(bullet_speed.x, bullet_speed.y);
				enemy_bullets.push_back(bullet);
			}
		}

		if (Event.type == sf::Event::KeyPressed)
		{
			//if player presses escape close game and disconnect them from server
			if (Event.key.code == sf::Keyboard::Escape)
			{
				music.stop();
				udp.unbind();
				tcp.disconnect();
				window.close();
				disconnect = true;
			}
		}
	}

	//if timer is less than 3 then show countdown on screen and return from update function to prevent gameplay from starting until countdown has finished 
	if (synced_timer <3)
	{
		text_time.setString(std::to_string(3-int(synced_timer)));
		text_time.setPosition(window.getSize().x/2 - text_time.getGlobalBounds().width/2, window.getSize().y / 2 - text_time.getGlobalBounds().height / 2);
		return;
	}
	//if gameplay has started update on screen timer
	else
	{
		text_time.setString(std::to_string(synced_timer));
		text_time.setPosition(window.getSize().x *0.1, window.getSize().y *0.9);
	}

	//resync timer every 10 seconds
	if (latency_timer.getElapsedTime().asSeconds() > 10)
	{

		//Get synced timer
		sf::Packet timer;
		PacketTypes type = PacketTypes::TimerSync;
		timer << static_cast<int>(type);

		//restart latency timer
		latency_timer.restart();
		tcp.send(timer);
	}

	//set player speed
	int speed = 3;
	if (players[ID - 1].alive and focused)//focused is to ensure only the active window is taking inputs when running multiple clients from same pc
	{
		//W sets player y speed to negative speed value (up)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
		{
			if (players[ID - 1].getPosition().y > 0)//keeps player in bounds
			{
				players[ID - 1].speed = sf::Vector2f(players[ID - 1].speed.x, -speed);
			}
		}
		//A sets player x speed to negative speed value (left)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
		{
			if (players[ID - 1].getPosition().x > 0)//keeps player in bounds
			{
				players[ID - 1].speed = sf::Vector2f(-speed, players[ID - 1].speed.y);
			}
		}
		//S sets player y speed to positive speed value (down)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
		{
			if (players[ID - 1].getPosition().y + players[ID - 1].getSize().y < window.getSize().y)//keeps player in bounds
			{
				players[ID - 1].speed = sf::Vector2f(players[ID - 1].speed.x, speed);
			}
		}
		//D sets player x speed to positive speed value (right)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
		{
			if (players[ID - 1].getPosition().x + players[ID - 1].getSize().x < window.getSize().x)//keeps player in bounds
			{
				players[ID - 1].speed = sf::Vector2f(speed, players[ID - 1].speed.y);
			}
		}

		//allow player to shootwhen cooldown has finished
		if (shoot_cooldown < 0)
		{
			bool shoot = false;
			Bullet_Data bullet_data;

			//when an arrow key is pressed set bullets position and speed appropriatley
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
			{
				shoot = true;
				bullet_data.xpos = players[ID - 1].getPosition().x + players[ID - 1].getSize().x / 2;
				bullet_data.ypos = players[ID - 1].getPosition().y;
				bullet_data.xspeed = 0;
				bullet_data.yspeed = -300;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
			{
				shoot = true;
				bullet_data.xpos = players[ID - 1].getPosition().x + players[ID - 1].getSize().x / 2;
				bullet_data.ypos = players[ID - 1].getPosition().y + players[ID - 1].getSize().y;
				bullet_data.xspeed = 0;
				bullet_data.yspeed = 300;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
			{
				shoot = true;
				bullet_data.xpos = players[ID - 1].getPosition().x;
				bullet_data.ypos = players[ID - 1].getPosition().y + players[ID - 1].getSize().y / 2;
				bullet_data.xspeed = -300;
				bullet_data.yspeed = 0;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
			{
				shoot = true;
				bullet_data.xpos = players[ID - 1].getPosition().x + players[ID - 1].getSize().x;
				bullet_data.ypos = players[ID - 1].getPosition().y + players[ID - 1].getSize().y / 2;
				bullet_data.xspeed = 300;
				bullet_data.yspeed = 0;
			}

			//if a key was pressed create new bullet
			if (shoot)
			{
				//reset cooldown and shoot bool
				shoot = false;
				shoot_cooldown = 0.5;

				//set pos and speed of bullet
				Bullet bullet(sf::Vector2f(bullet_data.xpos, bullet_data.ypos));
				bullet.speed = sf::Vector2f(bullet_data.xspeed, bullet_data.yspeed);

				//set bullets colour based on local ID
				switch (ID)
				{
				case 1:
					bullet.setFillColor(sf::Color::Red);
					break;
				case 2:
					bullet.setFillColor(sf::Color::Blue);
					break;
				case 3:
					bullet.setFillColor(sf::Color::Green);
					break;
				case 4:
					bullet.setFillColor(sf::Color::Magenta);
					break;
				default:
					break;
				}

				//add bullet to vector
				bullets.push_back(bullet);

				//send bullet data to server so other clients can create the bullet
				sf::Packet bull_data;
				PacketTypes type = PacketTypes::BulletFired;
				bull_data << static_cast<int>(type) << ID << bullet_data.xpos << bullet_data.ypos << bullet_data.xspeed << bullet_data.yspeed << synced_timer;
				tcp.send(bull_data);
			}
		}
	}

	//move player
	players[ID - 1].move(players[ID - 1].speed);
	players[ID - 1].speed.x = 0;
	players[ID - 1].speed.y = 0;

	//send position to server every 0.1 seconds
	if (packet_cooldown < 0)
	{
		sf::Packet posdata;
		posdata << ID << synced_timer << players[ID - 1].getPosition().x << players[ID - 1].getPosition().y;
		udp.send(posdata, server_ip, server_udp_port);
		packet_cooldown = 0.075;
	}

	//update players bullets
	for (size_t i = 0; i < bullets.size(); i++)
	{
		//move bullets
		bullets[i].move(bullets[i].speed * dt);

		//delete off-screen bullets
		if (bullets[i].getPosition().x <0 or bullets[i].getPosition().x>window.getSize().x or bullets[i].getPosition().y < 0 or bullets[i].getPosition().y>window.getSize().y)
		{
			bullets.erase(bullets.begin() + i);	
		}
	}

	//update enemies bullets
	for (size_t i = 0; i < enemy_bullets.size(); i++)
	{
		//move bullets
		enemy_bullets[i].move(enemy_bullets[i].speed * dt);

		//detect collison with enemy bullet
		if (players[ID - 1].alive)
		{
			if (enemy_bullets[i].getPosition().x + enemy_bullets[i].getSize().x > players[ID - 1].getPosition().x and enemy_bullets[i].getPosition().x < players[ID - 1].getPosition().x + players[ID - 1].getSize().x)
			{
				if (enemy_bullets[i].getPosition().y + enemy_bullets[i].getSize().y > players[ID - 1].getPosition().y and enemy_bullets[i].getPosition().y < players[ID - 1].getPosition().y + players[ID - 1].getSize().y)
				{
					//update local dead bool
					players[ID - 1].alive = false;

					//calc number of players still alive
					int num_alive_players = 0;
					for (int i = 0; i < player_count; i++)
					{
						if (players[i].alive)
						{
							num_alive_players++;
						}
					}

					//check if there is a winner
					bool winner = false;
					if (num_alive_players < 2)
					{
						winner = true;
					}

					//send data to server so other clients can set player to dead and server can know if there is winner
					sf::Packet dead;
					PacketTypes type = PacketTypes::BulletCollision;
					dead << static_cast<int>(type) << ID << false << winner;
					tcp.send(dead);

					//if there is a winner change to win state
					if (winner)
					{
						state = States::win;
						init = true;
						music.stop();
						return;
					}
				}
			}
		}

		//delete off-screen bullets
		if (enemy_bullets[i].getPosition().x <0 or enemy_bullets[i].getPosition().x > window.getSize().x or enemy_bullets[i].getPosition().y < 0 or enemy_bullets[i].getPosition().y>window.getSize().y)
		{
			enemy_bullets.erase(enemy_bullets.begin() + i);
		}
	}

	

	//loop through player count and calculate all predicted positions
	for (int i = 0; i < player_count; i++)
	{
		if (i != ID-1)
		{
			float displacementX = 0;
			float displacementY = 0;
			//check that 2nd packet has been recieved before doing prediction to ensure we have enough data to work with
			if (players[i].pos_data[1].time!=0)
			{
				//linear interpolation
				float final_distanceX = players[i].pos_data[0].x - players[i].pos_data[1].x;
				float final_distanceY = players[i].pos_data[0].y - players[i].pos_data[1].y;

				float final_speedX = final_distanceX / (players[i].pos_data[0].time - players[i].pos_data[1].time);
				float final_speedY = final_distanceY / (players[i].pos_data[0].time - players[i].pos_data[1].time);

				displacementX = final_speedX * (players[i].pos_data[0].time - players[i].pos_data[1].time);
				displacementY = final_speedY * (players[i].pos_data[0].time - players[i].pos_data[1].time);

				//QUADRATIC INTERPOLATION

				//float final_distanceX = players[i].pos_data[0].x - players[i].pos_data[1].x;
				//float final_distanceY = players[i].pos_data[0].y - players[i].pos_data[1].y;

				//float final_speedX = final_distanceX / (players[i].pos_data[0].time - players[i].pos_data[1].time);
				//float final_speedY = final_distanceY / (players[i].pos_data[0].time - players[i].pos_data[1].time);

				//float initial_distanceX = players[i].pos_data[1].x - players[i].pos_data[2].x;
				//float initial_distanceY = players[i].pos_data[1].y - players[i].pos_data[2].y;

				//float initial_speedX = initial_distanceX / (players[i].pos_data[1].time - players[i].pos_data[2].time);
				//float initial_speedY = initial_distanceY / (players[i].pos_data[1].time - players[i].pos_data[2].time);

				//float accelerationX = final_speedX - initial_speedX / (players[i].pos_data[0].time - players[i].pos_data[2].time);
				//float accelerationY = final_speedY - initial_speedY / (players[i].pos_data[0].time - players[i].pos_data[2].time);

				//displacementX = initial_speedX * (synced_timer - players[i].pos_data[0].time) + 0.5 * accelerationX * (synced_timer - players[i].pos_data[0].time) * (synced_timer - players[i].pos_data[0].time);
				//displacementY = initial_speedY * (synced_timer - players[i].pos_data[0].time) + 0.5 * accelerationY * (synced_timer - players[i].pos_data[0].time) * (synced_timer - players[i].pos_data[0].time);
			}
			//update other players positions
			predictedX = players[i].pos_data[0].x + displacementX;
			predictedY = players[i].pos_data[0].y + displacementY;
			players[i].setPosition(sf::Vector2f(predictedX, predictedY));
		}
	}
}

void Game::draw(sf::RenderWindow& window, int& player_count, Player players[4])
{
	//draw background, players, bullets, timer
	window.clear();
	window.draw(backgroundSprite);
	if (synced_timer < 3)
	{
		window.draw(text_time);
	}
	for (int i = 0; i < player_count; i++)
	{
		if (players[i].alive)
		{
			window.draw(players[i]);
		}
	}
	for (size_t i = 0; i < bullets.size(); i++)
	{
		window.draw(bullets[i]);
	}
	for (size_t i = 0; i < enemy_bullets.size(); i++)
	{
		window.draw(enemy_bullets[i]);
	}
	window.draw(text_time);
	window.display();
}
