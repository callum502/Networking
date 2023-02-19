#include "Win.h"

Win::Win()
{
	//load font
	if (!font.loadFromFile("arial.ttf"))
	{
		std::cout << "error loading font" << std::endl;
	}

	//load confetti texture
	if (!confettiTexture.loadFromFile("win.jpg"))
	{
		std::cout << "error loading background texture" << std::endl;
	}

	//load hat texture
	if (!hatTexture.loadFromFile("hat.png"))
	{
		std::cout << "error loading background texture" << std::endl;
	}
}

void Win::init(bool& init, Player players[4], int player_count, sf::RenderWindow& window)
{
	init = false;

	//set player sizes
	players[0].setSize(sf::Vector2f(100, 100));
	players[1].setSize(sf::Vector2f(100, 100));
	players[2].setSize(sf::Vector2f(100, 100));
	players[3].setSize(sf::Vector2f(100, 100));

	//calc winner
	for (int i = 0; i < player_count; i++)
	{
		if (players[i].alive)
		{
			winner_ID = i;
			players[i].setPosition(window.getSize().x * 0.5 - players[i].getSize().x * 0.5, window.getSize().y * 0.5 - players[i].getSize().y * 0.5);
		}
	}

	//setup text
	winner_text.setFont(font);
	std::string text;
	text = players[winner_ID].name;
	text.append(" won!!!");
	winner_text.setString(text);
	winner_text.setCharacterSize(24);
	winner_text.setStyle(sf::Text::Bold | sf::Text::Italic);
	winner_text.setPosition(window.getSize().x / 2 - winner_text.getGlobalBounds().width / 2, window.getSize().y *0.7);
	winner_text.setFillColor(sf::Color::Red);

	//setup enter text
	enter_text.setFont(font);
	enter_text.setString("Press enter to return to lobby");
	winner_text.setCharacterSize(24);
	winner_text.setStyle(sf::Text::Bold | sf::Text::Italic);
	winner_text.setPosition(window.getSize().x / 2 - winner_text.getGlobalBounds().width / 2, window.getSize().y * 0.8);
	winner_text.setFillColor(sf::Color::Red);

	//setup confetti sprite
	confettiSprite.setTexture(confettiTexture);
	confettiSprite.setPosition(window.getSize().x * 0.5 - confettiSprite.getGlobalBounds().width * 0.5, window.getSize().y * 0.5 - confettiSprite.getGlobalBounds().height * 0.5);

	//setup hat sprite
	hatSprite.setScale(sf::Vector2f(0.5,0.5));
	hatSprite.setTexture(hatTexture);
	hatSprite.setPosition(window.getSize().x * 0.5 - hatSprite.getGlobalBounds().width * 0.5, players[winner_ID].getPosition().y - hatSprite.getGlobalBounds().height);
}

void Win::update(sf::RenderWindow& window, sf::Event Event, States& state, Player players[4], int player_count, bool& init)
{
	//change to lobby state when player presses enter
	if (Event.type == sf::Event::KeyPressed)
	{
		if (Event.key.code == sf::Keyboard::Enter)
		{
			state = States::lobby;
			init = true;
		}
	}
}

void Win::draw(sf::RenderWindow& window, int player_count, Player players[4])
{
	//draw confetti, hat, player and text
	window.clear();
	window.draw(confettiSprite);
	window.draw(hatSprite);
	window.draw(players[winner_ID]);
	window.draw(winner_text);
	window.display();
}
