#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <list>
#include <vector>

//packet types so we can determine which type of packet has been recieved
enum class PacketTypes { PlayerData, ReadyToggle, StartGame, TimerSync, PlayerPositions, BulletFired, BulletCollision };

//player struct to keep track of each players data server side
struct player {
    std::string name;
    bool ready=false;
    int ID=0;
    bool disconnected = false;
    bool alive = true;
    sf::TcpSocket* client;
    
    //udp address
    sf::IpAddress sender;
    unsigned short port;
};

int main()
{
//init variables
std::vector<player> players;
bool in_game = false;
bool can_start = false;
int port;

//setup udp socket
sf::UdpSocket udp;
udp.bind(sf::Socket::AnyPort);
udp.setBlocking(false);

// Create a socket to listen to new connections
sf::TcpListener listener;
listener.listen(55001);

// Create a selector
sf::SocketSelector selector;

// Add the listener to the selector
selector.add(listener);

    //LOBBY LOOP
    std::cout << "awaiting connections" << std::endl << std::endl;
    while (true)
    {
        while (!in_game)
        {
            // wait until a socket is ready
            if (selector.wait())
            {
                // Test the listener
                if (selector.isReady(listener))
                {
                    // The listener is ready: there is a pending connection
                    sf::TcpSocket* client = new sf::TcpSocket;
                    client->setBlocking(false);

                    if (listener.accept(*client) == sf::Socket::Done)
                    {
                        if (players.size() < 4)
                        {
                            //Add a new player with the connected socket 
                            player temp_player;
                            temp_player.client = client;
                            players.push_back(temp_player);
                            // Add the new client to the selector so that we will be notified when they send a packet
                            selector.add(*client);
                            //set can start to false as there is a new player who is not ready yet
                            can_start = false;

                            //send ID back to client who connected
                            sf::Packet p_ID;
                            p_ID << players.size() << udp.getLocalPort();
                            client->send(p_ID);
                        }
                        else
                        {
                            //already at max players
                            delete client;
                        }
                    }
                    else
                    {
                        // Error, we won't get a new connection, delete the socket
                        delete client;
                    }
                }
                else
                {
                    // The listener socket is not ready, test all other sockets (the clients)
                    for (std::vector<player>::iterator it = players.begin(); it != players.end(); ++it)
                    {
                        sf::TcpSocket& client = *it->client;
                        //if current client has sent a packet that is ready to be recieved
                        if (selector.isReady(client))
                        {
                            //recieve the packet
                            sf::Packet packet;
                            if (client.receive(packet) == sf::Socket::Done)
                            {
                                //extract the packet type to determine what to do with packet
                                int type = -1;
                                packet >> type;

                                //if recieved packet is the clients name
                                if (static_cast<PacketTypes>(type) == PacketTypes::PlayerData)
                                {
                                    //set the players name to what the client has sent
                                    std::string ip;
                                    packet >> it->name >> it->ID >> ip >> it->port;
                                    it->sender = static_cast<sf::IpAddress>(ip);
                                    std::cout << it->name << " connected" << std::endl;
                                }

                                //If recieved packet is a ready toggle
                                else if (static_cast<PacketTypes>(type) == PacketTypes::ReadyToggle)
                                {
                                    //extract data from packet
                                    bool ready;
                                    int ID;
                                    packet >> ID >> ready >> in_game;

                                    //update players ready status
                                    it->ready = ready;

                                    //if all players are ready and there is more than one player then set can_start to true
                                    if (players.size() > 1)
                                    {
                                        can_start = true;
                                        for (std::vector<player>::iterator it = players.begin(); it != players.end(); ++it)
                                        {
                                            if (!it->ready)
                                            {
                                                can_start = false;
                                            }
                                        }
                                    }
                                }
                            }
                            //check for client dissconnection
                            else if (client.receive(packet) == sf::Socket::Disconnected)
                            {
                                //display that player has left
                                std::cout << it->name << " disconnected" << std::endl;

                                //remove client
                                selector.remove(client);
                                client.disconnect();
                                delete &client;
                                players.erase(it);

                                //re-configure client IDs
                                int updated_ID = 1;
                                for (std::vector<player>::iterator player = players.begin(); player != players.end(); ++player)
                                {
                                    sf::TcpSocket& client = *player->client;
                                    //update server side ID
                                    player->ID = updated_ID;

                                    //send client new ID
                                    sf::Packet new_ID;
                                    int packet_type = 0;
                                    new_ID << packet_type << updated_ID;
                                    client.send(new_ID);

                                    updated_ID++;
                                }
                                //break to prevent continuing to iterate through player vector after deleting a player
                                break;
                            }
                        }
                    }
                }
                //after socket recieves data update lobby data
                for (std::vector<player>::iterator it = players.begin(); it != players.end(); ++it)
                {
                    sf::TcpSocket& client = *it->client;
                    sf::Packet lobbydata;
                    //populate packet
                    int packet_type = 1;
                    lobbydata << packet_type << players.size();
                    //iterate through players and populate packet with the server side player data
                    for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
                    {
                        lobbydata << it2->name;
                        lobbydata << it2->ready;
                        lobbydata << it2->ID;
                    }
                    lobbydata << can_start;
                    lobbydata << in_game;
                    //send clients updated lobby data
                    client.send(lobbydata);
                }

            }
        }
        selector.add(udp);

        //setup variables
        sf::Clock timer;
        timer.restart();
        can_start = false;

        //GAME LOOP
        while (in_game)
        {
            if (selector.wait())
            {
                //when we recieve data check the udp socket
                if (selector.isReady(udp))
                {
                    sf::Packet posData;
                    unsigned short port;
                    sf::IpAddress sender;
                    udp.receive(posData, sender, port);
                    for (std::vector<player>::iterator it = players.begin(); it != players.end(); ++it)
                    {
                         udp.send(posData, it->sender, it->port);          
                    }
                }
                else
                {
                    //the udp socket is not ready so check the tcp sockets
                    for (std::vector<player>::iterator it = players.begin(); it != players.end(); ++it)
                    {
                        sf::TcpSocket& client = *it->client;
                        if (selector.isReady(client))
                        {
                            //client has sent tcp data attempt to recieve it
                            sf::Packet tcp_packet;
                            if (client.receive(tcp_packet) == sf::Socket::Done)
                            {
                                //get packet type
                                int packet_type = -1;
                                tcp_packet >> packet_type;

                                //if client sent timer resync packet
                                if (static_cast<PacketTypes>(packet_type) == PacketTypes::TimerSync)
                                {
                                    //send client the current timer value
                                    sf::Packet timer_packet;
                                    PacketTypes type = PacketTypes::TimerSync;
                                    timer_packet << static_cast<int>(type) << timer.getElapsedTime().asSeconds();
                                    client.send(timer_packet);
                                }
                                //if client sent bullet collision packet
                                else if (static_cast<PacketTypes>(packet_type) == PacketTypes::BulletCollision)
                                {
                                    //extract variables
                                    int rec_ID = -1;
                                    bool rec_alive;
                                    bool winner = false;
                                    tcp_packet >> rec_ID >> rec_alive >> winner;

                                    //update server side alive status
                                    it->alive = rec_alive;

                                    //if there is a winner switch back to lobby state
                                    if (winner)
                                    {
                                        in_game = false;
                                        //iterate through players make them all not ready
                                        for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
                                        {
                                            it2->ready = false;
                                        }
                                        selector.remove(udp);
                                    }

                                    //return packet to other clients so they can update the players alive status
                                    sf::Packet return_packet;
                                    PacketTypes type = PacketTypes::BulletCollision;
                                    return_packet << static_cast<int>(type) << rec_ID << rec_alive;
                                    for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
                                    {
                                        sf::TcpSocket& client2 = *it2->client;
                                        if (it != it2)
                                        {
                                            client2.send(return_packet);
                                        }
                                    }
                                }
                                //if client sent bullet fired packet
                                else if (static_cast<PacketTypes>(packet_type) == PacketTypes::BulletFired)
                                {
                                    //return packet to other clients so they can create the bullet that has been fired
                                    sf::Vector2f bullet_pos;
                                    sf::Vector2f bullet_speed;
                                    int rec_ID = -1;
                                    float time_sent;
                                    tcp_packet >> rec_ID >> bullet_pos.x >> bullet_pos.y >> bullet_speed.x >> bullet_speed.y >> time_sent;
                                    sf::Packet return_packet;
                                    PacketTypes type = PacketTypes::BulletFired;
                                    return_packet << static_cast<int>(type) << rec_ID << bullet_pos.x << bullet_pos.y << bullet_speed.x << bullet_speed.y << time_sent;
                                    for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
                                    {
                                        sf::TcpSocket& client2 = *it2->client;
                                        if (it != it2)
                                        {
                                            client2.send(return_packet);
                                        }
                                    }
                                }
                            }
                            //if the client has disconnected during game
                            else if (client.receive(tcp_packet) == sf::Socket::Disconnected)
                            {
                                //display that player has left
                                std::cout << it->name << " disconnected" << std::endl;

                                //tell the other clients to update the players alive status
                                sf::Packet return_packet;
                                PacketTypes type = PacketTypes::BulletCollision;
                                return_packet << static_cast<int>(type) << it->ID << false;
                                for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
                                {
                                    sf::TcpSocket& client2 = *it2->client;
                                    if (it != it2)
                                    {
                                        client2.send(return_packet);
                                    }
                                }

                                //update server side alive status of player
                                it->alive = false;

                                //determine if there is a winner
                                int num_alive_players = 0;
                                for (int i = 0; i < players.size(); i++)
                                {
                                    if (players[i].alive)
                                    {
                                        num_alive_players++;
                                    }
                                }

                                //if winner then setup for returning to lobby
                                if (num_alive_players < 2)
                                {
                                    in_game = false;
                                    //iterate through players make them all not ready
                                    for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
                                    {
                                        it2->ready = false;
                                    }
                                    selector.remove(udp);
                                }

                                //delete client from server
                                selector.remove(client);
                                client.disconnect();
                                delete& client;
                                players.erase(it);
                                break;
                            }
                        }
                    }
                }
            }

        }
        //game has ended
        //re-configure IDs before returning to lobby
        int updated_ID = 1;
        for (std::vector<player>::iterator player = players.begin(); player != players.end(); ++player)
        {
            sf::TcpSocket& client = *player->client;

            //update server side ID
            player->ID = updated_ID;

            //send client new ID
            sf::Packet new_ID;
            int packet_type = 0;
            new_ID << packet_type << updated_ID;
            client.send(new_ID);

            //reset player alive bool
            player->alive = true;

            updated_ID++;
        }

        //update lobby data 
        for (std::vector<player>::iterator it = players.begin(); it != players.end(); ++it)
        {
            sf::TcpSocket& client = *it->client;
            sf::Packet lobbydata;
            //populate packet
            int packet_type = 1;
            lobbydata << packet_type << players.size();
            //iterate through players and populate packet with the server side player data
            for (std::vector<player>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
            {
                lobbydata << it2->name;
                lobbydata << it2->ready;
                lobbydata << it2->ID;
            }
            lobbydata << can_start;
            lobbydata << in_game;
            //send clients updated lobby data
            client.send(lobbydata);
        }
    }
}